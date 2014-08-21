

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
#include <ramdisk/include/services/common/RdmaDevice.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <ramdisk/include/services/common/logging.h>
#include <ramdisk/include/services/common/Cioslog.h>
#include <iostream>
#include <iomanip>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
//#include <stdint.h>

#include <mutex>
#include <thread>

std::mutex alloc_mutex;

#define RDMA_USE_MMAP 1

using namespace bgcios;



/*---------------------------------------------------------------------------*/
//! Special memory region structure returned by the bgvrnic device.
struct bgvrnic_mr
{
   struct ibv_mr region;     //! Standard verbs memory region.
   uint32_t num_frags;       //! Number of fragments in the physical pages used by the memory region.
};

#define BGVRNIC_STRUCT(ptr) ((bgvrnic_mr*)(ptr))
#define IBV_MR_STRUCT(ptr)  ((ibv_mr*)(ptr))

LOG_DECLARE_FILE("cios.common");
/*---------------------------------------------------------------------------*/
RdmaMemoryRegion::RdmaMemoryRegion(RdmaProtectionDomainPtr pd, const void *buffer, const uint64_t length)
{
  _messageLength = length;
  _frags = -1; // frags = -1 is a special flag we use to tell destructor not to deallocate the memory on close
  _fd = -1;
  int accessFlags = _IBV_ACCESS_LOCAL_WRITE|_IBV_ACCESS_REMOTE_WRITE|_IBV_ACCESS_REMOTE_READ;

  //  void *buffer2 = ::mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);

  _region = ibv_reg_mr(pd->getDomain(), (void*)buffer, _messageLength, (ibv_access_flags)accessFlags);
  if (_region == NULL) {
    int err = errno;
    LOG_ERROR_MSG("error registering ibv_reg_mr error/message: " << err << "/" << bgcios::errorString(err));
  }
  else {
    LOG_DEBUG_MSG("OK registering memory ="
        << std::setw(8) << std::setfill('0') << std::hex << buffer << " : "
        << std::setw(8) << std::setfill('0') << std::hex << _region->addr
        << " length " << length);
  }
}

/*---------------------------------------------------------------------------*/
int 
RdmaMemoryRegion::allocate(RdmaProtectionDomainPtr pd, size_t length)
{
   // Obtain lock to serialize getting storage for memory regions.  This makes it more likely that we'll get contiguous physical pages
   // for the memory region (maybe someday we'll be able to use huge pages).
  std::lock_guard<std::mutex> lock(alloc_mutex);

  // _allocateLock->lock();

   // Allocate memory regions until we get one that is not fragmented or there are too many attempts.
   struct ibv_mr *regionList[MaxAllocateAttempts];
  struct ibv_mr *memoryRegion;
   uint32_t attempt = 0;

   do {
      // Allocate storage for the memory region.
#ifdef RDMA_USE_MMAP
      void *buffer = ::mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (buffer != MAP_FAILED) {
      LOG_DEBUG_MSG("allocated storage for memory region with mmap OK, vlength = " << length);
    }
    else if (buffer == MAP_FAILED) {
         int err = errno;
      LOG_ERROR_MSG("error allocating storage using mmap for memory region: " << length << " " << bgcios::errorString(err));
      //         _allocateLock->unlock();
         return err;
      }
#else
    void *buffer = malloc(length);
    if (buffer != NULL) {
      LOG_DEBUG_MSG("allocated storage for memory region with malloc OK " << length);
    }
#endif

      // Register the storage as a memory region.
      int accessFlags = _IBV_ACCESS_LOCAL_WRITE|_IBV_ACCESS_REMOTE_WRITE|_IBV_ACCESS_REMOTE_READ;
    std::bitset<sizeof(int)*8> bits(accessFlags);
      regionList[attempt] = ibv_reg_mr(pd->getDomain(), buffer, length, (ibv_access_flags)accessFlags);

      if (regionList[attempt] == NULL) {
      int err = errno;
      LOG_ERROR_MSG("error registering ibv_reg_mr error message with flags : " << bits << " " << err << " " << bgcios::errorString(err));
      //         _allocateLock->unlock();
         return ENOMEM;
      }
    else {
      LOG_DEBUG_MSG("OK registering ibv_reg_mr with flags : " << bits << " " << length);
    }

    memoryRegion = regionList[attempt];
      ++attempt;

    // if not using the bgqvrnic device, one attempt is enough
    // but, if we are on BGQ we must check for non fragmented regions
    //
    // Check on the number of fragments in the memory region.
  } while ((RdmaDevice::bgvrnic_device) && (BGVRNIC_STRUCT(memoryRegion)->num_frags > 1) && (attempt < MaxAllocateAttempts));

  //
  // If it took multiple attempts to get an unfragmented memory region.
  // find the memory region with the smallest number of fragments.
  //
  if (RdmaDevice::bgvrnic_device) {
    LOG_DEBUG_MSG("Memory allocation using special bgvrnic device properties");
   if (attempt > 1) {
      uint32_t minFragments = 100;
      uint32_t bestRegion = 0;
      for (uint32_t index = 0; index < attempt; ++index) {
        memoryRegion = regionList[index];
        if (BGVRNIC_STRUCT(memoryRegion)->num_frags < minFragments) {
            bestRegion = index;
          minFragments = BGVRNIC_STRUCT(memoryRegion)->num_frags;
         }
      }
      if (minFragments > 1) {
         LOG_CIOS_INFO_MSG("memory region with local key " << regionList[bestRegion]->lkey << " at address " << regionList[bestRegion]->addr <<
                      " with length " << regionList[bestRegion]->length << " has "<< minFragments << " fragments"); 
      }

      // Release all of the memory regions except for the best one.
      for (uint32_t index = 0; index < attempt; ++index) {
         if (index != bestRegion) {
          memoryRegion = regionList[index];
          CIOSLOGRDMA_REQ(BGV_RDMADROP, regionList[index],(int)BGVRNIC_STRUCT(memoryRegion)->num_frags,_fd);
            void *buffer = regionList[index]->addr;
            size_t length = regionList[index]->length;
            ibv_dereg_mr(regionList[index]);
            ::munmap(buffer, length);
         }
      }

      // Use the best region.
      _region = regionList[bestRegion];
      memoryRegion = regionList[bestRegion];
      _frags = (int)BGVRNIC_STRUCT(memoryRegion)->num_frags;
   }
    else {
   // We got an unfragmented region on the first try.
      _region = regionList[0];
      _frags = (int)BGVRNIC_STRUCT(regionList[0])->num_frags;
    }
  }
   else {
    // a normal device does not return fragmented regions
      _region = regionList[0];
    _frags = 1;
   }

   // Release the lock.
  //   _allocateLock->unlock();

   LOG_CIOS_DEBUG_MSG("allocated memory region with local key " << getLocalKey() << " at address " << getAddress() << " with length " << getLength());
   CIOSLOGRDMA_REQ(BGV_RDMA_REG,_region,_frags,_fd);
   return 0;
}

/*---------------------------------------------------------------------------*/
int 
RdmaMemoryRegion::allocate64kB(RdmaProtectionDomainPtr pd)
{
      size_t length = 65536;
      // Allocate storage for the memory region.
#ifdef RDMA_USE_MMAP
      void *buffer = ::mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
  if (buffer != MAP_FAILED) {
    LOG_DEBUG_MSG("allocated storage for memory region with mmap OK " << length);
  }
#else
  void *buffer = malloc(length);
  if (buffer != NULL) {
    LOG_DEBUG_MSG("allocated storage for memory region with malloc OK " << length);
  }
#endif
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
    int err = errno;
    LOG_ERROR_MSG("ibv_reg_mr error message : " << bgcios::errorString(err));
         ::munmap(buffer, length);
         return ENOMEM;
      }
      struct bgvrnic_mr * bgv_region = (struct bgvrnic_mr * )_region;
      _frags = (int)bgv_region->num_frags;
      //logCRdmaReg(BGV_RDMA_REG, _region->addr, _region->length, _region->lkey, _frags, _fd );
      CIOSLOGRDMA_REQ(BGV_RDMA_REG,_region,_frags,_fd);
      return 0;
}

/*---------------------------------------------------------------------------*/
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

/*---------------------------------------------------------------------------*/
int
RdmaMemoryRegion::release(void)
{
   if (_region != NULL) {
      CIOSLOGRDMA_REQ(BGV_RDMA_RMV,_region,_frags,_fd);
//      LOG_CIOS_DEBUG_MSG("released memory region with local key " << getLocalKey() << " at address " << buffer << " with length " << length);
      void *buffer = getAddress();
      uint32_t length = getLength();
      ibv_dereg_mr(_region);
    // _frags == -1 is special to tell us not to release the memory, just unregister it
    if (_frags!=-1) {
#ifdef RDMA_USE_MMAP
      ::munmap(buffer, length);
#else
      free(buffer);
#endif
    }
      _region = NULL;
   }

   return 0;
}

/*---------------------------------------------------------------------------*/
std::ostream&
RdmaMemoryRegion::writeTo(std::ostream& os) const
{
   os << "addr=" << getAddress() << " length=" << getLength() << " lkey=" << getLocalKey();
   os << " rkey=" << getRemoteKey() << "msglen= " << getMessageLength();
   return os;
}

