

/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (C) Copyright IBM Corp.  2011, 2012                              */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */

//! \file  RdmaMemoryRegion.cc
//! \brief Methods for bgcios::RdmaMemoryRegion class.

// Includes
#include <ramdisk/include/services/common/RdmaMemoryRegion.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <ramdisk/include/services/common/logging.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ramdisk/include/services/common/Cioslog.h>

using namespace bgcios;

//! Special memory region structure returned by the bgvrnic device.

struct bgvrnic_mr
{
   struct ibv_mr region;     //! Standard verbs memory region.
   uint32_t num_frags;       //! Number of fragments in the physical pages used by the memory region.
};

LOG_DECLARE_FILE("cios.common");

int 
RdmaMemoryRegion::allocate(RdmaProtectionDomainPtr pd, size_t length)
{
   // Obtain lock to serialize getting storage for memory regions.  This makes it more likely that we'll get contiguous physical pages
   // for the memory region (maybe someday we'll be able to use huge pages).
   _allocateLock->lock();

   // Allocate memory regions until we get one that is not fragmented or there are too many attempts.
   struct ibv_mr *regionList[MaxAllocateAttempts];
   struct bgvrnic_mr *memoryRegion;
   uint32_t attempt = 0;

   do {
      // Allocate storage for the memory region.
      void *buffer = ::mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
      if (buffer == MAP_FAILED) {
         int err = errno;
         LOG_ERROR_MSG("error allocating storage for memory region: " << bgcios::errorString(err));
         _allocateLock->unlock();
         return err;
      }

      // Register the storage as a memory region.
      int accessFlags = _IBV_ACCESS_LOCAL_WRITE|_IBV_ACCESS_REMOTE_WRITE|_IBV_ACCESS_REMOTE_READ;
      regionList[attempt] = ibv_reg_mr(pd->getDomain(), buffer, length, (ibv_access_flags)accessFlags);
      if (regionList[attempt] == NULL) {
         LOG_ERROR_MSG("error registering memory region");
         ::munmap(buffer, length);
         _allocateLock->unlock();
         return ENOMEM;
      }

      // Check on the number of fragments in the memory region.
      memoryRegion = (struct bgvrnic_mr *)regionList[attempt];
      ++attempt;

   } while ((memoryRegion->num_frags > 1) && (attempt < MaxAllocateAttempts));

   // It took multiple attempts to get an unfragmented memory region.  Find the memory region with the smallest number of fragments.
   if (attempt > 1) {
      uint32_t minFragments = 100;
      uint32_t bestRegion = 0;
      for (uint32_t index = 0; index < attempt; ++index) {
         memoryRegion = (struct bgvrnic_mr *)regionList[index];
         if (memoryRegion->num_frags < minFragments) {
            bestRegion = index;
            minFragments = memoryRegion->num_frags;
         }
      }
      if (minFragments > 1) {
         LOG_CIOS_INFO_MSG("memory region with local key " << regionList[bestRegion]->lkey << " at address " << regionList[bestRegion]->addr <<
                      " with length " << regionList[bestRegion]->length << " has "<< minFragments << " fragments"); 
      }

      // Release all of the memory regions except for the best one.
      for (uint32_t index = 0; index < attempt; ++index) {
         if (index != bestRegion) {
            memoryRegion = (struct bgvrnic_mr *)regionList[index];
            CIOSLOGRDMA_REQ(BGV_RDMADROP, regionList[index],(int)memoryRegion->num_frags,_fd);
            void *buffer = regionList[index]->addr;
            size_t length = regionList[index]->length;
            ibv_dereg_mr(regionList[index]);
            ::munmap(buffer, length);
         }
      }

      // Use the best region.
      _region = regionList[bestRegion];
      memoryRegion = (struct bgvrnic_mr *)regionList[bestRegion];
      _frags = (int)memoryRegion->num_frags;
   }

   // We got an unfragmented region on the first try.
   else {
      _region = regionList[0];
      memoryRegion = (struct bgvrnic_mr *)regionList[0];
      _frags = (int)memoryRegion->num_frags;
   }

   // Release the lock.
   _allocateLock->unlock();

   LOG_CIOS_DEBUG_MSG("allocated memory region with local key " << getLocalKey() << " at address " << getAddress() << " with length " << getLength());
   CIOSLOGRDMA_REQ(BGV_RDMA_REG,_region,_frags,_fd);
   return 0;
}

int 
RdmaMemoryRegion::allocate64kB(RdmaProtectionDomainPtr pd)
{
      size_t length = 65536;
      // Allocate storage for the memory region.
      void *buffer = ::mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
      if (buffer == MAP_FAILED) {
         int err = errno;
         LOG_ERROR_MSG("error allocating storage for memory region: " << bgcios::errorString(err));
         return err;
      }

      // Register the storage as a memory region.
      int accessFlags = _IBV_ACCESS_LOCAL_WRITE|_IBV_ACCESS_REMOTE_WRITE|_IBV_ACCESS_REMOTE_READ;
      _region = ibv_reg_mr(pd->getDomain(), buffer, length, (ibv_access_flags)accessFlags);
      if (_region == NULL) {
         LOG_ERROR_MSG("error registering memory region");
         ::munmap(buffer, length);
         return ENOMEM;
      }
      struct bgvrnic_mr * bgv_region = (struct bgvrnic_mr * )_region;
      _frags = (int)bgv_region->num_frags;
      //logCRdmaReg(BGV_RDMA_REG, _region->addr, _region->length, _region->lkey, _frags, _fd );
      CIOSLOGRDMA_REQ(BGV_RDMA_REG,_region,_frags,_fd);
      return 0;
}

int 
RdmaMemoryRegion::allocateFromBgvrnicDevice(RdmaProtectionDomainPtr pd, size_t length)
{
      unsigned long region_size;

      // Open I/O memory device.
      _fd = open("/dev/bgiomem",O_RDWR);
      if (_fd == -1){
         int err = errno;
         LOG_DEBUG_MSG("error allocating file desciptor for memory region mmap: " << bgcios::errorString(err));
         return err;
      }

      // Verify large region size.
      if (read(_fd, &region_size, sizeof(region_size)) == sizeof(region_size)) {
	if (length > region_size) {
		int err = EINVAL;
		close(_fd);
		LOG_WARN_MSG("Allocation length of " << length << " > large region size of " << region_size);
		return err;
	}	
      } else
	LOG_ERROR_MSG("Unable to verify large region size."); 
     
      // Allocate storage for the memory region.
      void *buffer = ::mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED, _fd, 0);
      if (buffer == MAP_FAILED) {
         int err = errno;
         close(_fd);
         LOG_WARN_MSG("mmap allocateFromBgvrnicDevice errno=" <<errno << " "<<bgcios::errorString(err));
         return err;
      }

      // Register the storage as a memory region.
      int accessFlags = _IBV_ACCESS_LOCAL_WRITE|_IBV_ACCESS_REMOTE_WRITE|_IBV_ACCESS_REMOTE_READ;
      _region = ibv_reg_mr(pd->getDomain(), buffer, length, (ibv_access_flags)accessFlags);
      if (_region == NULL) {
         LOG_WARN_MSG("ibv_reg_mr allocateFromBgvrnicDevice error registering memory region");
         ::munmap(buffer, length);
         close(_fd);
         return ENOMEM;
      }
      struct bgvrnic_mr * bgv_region = (struct bgvrnic_mr * )_region;
      _frags = (int)bgv_region->num_frags;
      close(_fd);
      CIOSLOGRDMA_REQ(BGV_RDMA_REG,_region,_frags,_fd);
      return 0;
}

int
RdmaMemoryRegion::release(void)
{
   if (_region != NULL) {
      CIOSLOGRDMA_REQ(BGV_RDMA_RMV,_region,_frags,_fd);
//      LOG_CIOS_DEBUG_MSG("released memory region with local key " << getLocalKey() << " at address " << buffer << " with length " << length);
      void *buffer = getAddress();
      uint32_t length = getLength();
      ibv_dereg_mr(_region);
      ::munmap(buffer, length);
      _region = NULL;
      LOG_CIOS_DEBUG_MSG("released memory region with local key " << getLocalKey() << " at address " << buffer << " with length " << length);
   }

   return 0;
}

std::ostream&
RdmaMemoryRegion::writeTo(std::ostream& os) const
{
   os << "addr=" << getAddress() << " length=" << getLength() << " lkey=" << getLocalKey();
   os << " rkey=" << getRemoteKey() << "msglen= " << getMessageLength();
   return os;
}

