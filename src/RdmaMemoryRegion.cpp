// Copyright (c) 2014-2015 John Biddiscombe
// Copyright (c) 2011,2012 IBM Corp.
//
// ================================================================
// Portions of this code taken from IBM BlueGene-Q source
// 
// This software is available to you under the
// Eclipse Public License (EPL).
//
// Please refer to the file "eclipse-1.0.txt" 
// ================================================================
//

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

#include "RdmaLogging.h"
#include <RdmaMemoryRegion.h>
#include <RdmaDevice.h>
#include <RdmaError.h>
//
#include <unistd.h>
//#include "stdio.h"
//#include "stdlib.h"
#include <ostream>
#include <sstream>
#include <memory>
#include <string>
#include <iostream>
#include <iomanip>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>

#include <mutex>
#include <thread>

std::mutex alloc_mutex;

#define RDMA_USE_MMAP 1

using namespace bgcios;

/*---------------------------------------------------------------------------*/
//! Special memory region structure returned by the bgvrnic device.
struct bgvrnic_mr {
  struct ibv_mr region;     //! Standard verbs memory region.
  uint32_t num_frags; //! Number of fragments in the physical pages used by the memory region.
};

#define BGVRNIC_STRUCT(ptr) ((bgvrnic_mr*)(ptr))
#define IBV_MR_STRUCT(ptr)  ((ibv_mr*)(ptr))

//LOG_DECLARE_FILE("cios.common");
/*---------------------------------------------------------------------------*/
RdmaMemoryRegion::RdmaMemoryRegion(RdmaProtectionDomainPtr pd,
    const void *buffer, const uint64_t length)
{
  _messageLength = length;
  _flags  =  BLOCK_USER;
  _frags  = -1;
  _fd     = -1;

  int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
      | _IBV_ACCESS_REMOTE_READ;

  _region = ibv_reg_mr(pd->getDomain(), (void*) buffer, _messageLength,
      (ibv_access_flags) accessFlags);
  if (_region == NULL) {
    int err = errno;
    LOG_ERROR_MSG(
        "error registering user mem ibv_reg_mr " << hexpointer(buffer) << " " << hexlength(length) << " error/message: " << err << "/"
            << RdmaError::errorString(err));
  } else {
    LOG_DEBUG_MSG(
        "OK registering memory =" << hexpointer(buffer) << " : "
        << hexpointer(_region->addr) << " length " << hexlength(length));
  }
}

/*---------------------------------------------------------------------------*/
int RdmaMemoryRegion::allocate(RdmaProtectionDomainPtr pd, size_t length) {
  // Obtain lock to serialize getting storage for memory regions.
  // This makes it more likely that we'll get contiguous physical pages
  // for the memory region (maybe someday we'll be able to use huge pages).
  std::lock_guard < std::mutex > lock(alloc_mutex);

  // Allocate memory regions until we get one that is not fragmented or there are too many attempts.
  struct ibv_mr *regionList[MaxAllocateAttempts];
  struct ibv_mr *memoryRegion;
  uint32_t attempt = 0;

  do {
    // Allocate storage for the memory region.
#ifdef RDMA_USE_MMAP
    void *buffer = ::mmap(NULL, length, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buffer != MAP_FAILED) {
      LOG_DEBUG_MSG(
          "allocated storage for memory region with mmap OK, vlength = "
              << hexnumber(length));
    } else if (buffer == MAP_FAILED) {
      int err = errno;
      LOG_ERROR_MSG(
          "error allocating storage using mmap for memory region: " << hexnumber(length)
              << " " << RdmaError::errorString(err));
      return err;
    }
#else
    void *buffer = malloc(length);
    if (buffer != NULL) {
      LOG_DEBUG_MSG("allocated storage for memory region with malloc OK " << hexnumber(length));
    }
#endif

    // Register the storage as a memory region.
    int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
        | _IBV_ACCESS_REMOTE_READ;
    regionList[attempt] = ibv_reg_mr(pd->getDomain(), buffer, length,
        (ibv_access_flags) accessFlags);

    if (regionList[attempt] == NULL) {
      int err = errno;
      LOG_ERROR_MSG(
          "error registering ibv_reg_mr error message : "
              << " " << err << " " << RdmaError::errorString(err));
      return ENOMEM;
    } else {
      LOG_DEBUG_MSG(
          "OK registering ibv_reg_mr with flags : " << " " << hexnumber(accessFlags));
    }

    memoryRegion = regionList[attempt];
    ++attempt;

    // if not using the bgqvrnic device, one attempt is enough
    // but, if we are on BGQ we must check for non fragmented regions
    //
    // Check on the number of fragments in the memory region.
  } while ((RdmaDevice::bgvrnic_device)
      && (BGVRNIC_STRUCT(memoryRegion)->num_frags > 1)
      && (attempt < MaxAllocateAttempts));

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
            " with length " << hexlength(regionList[bestRegion]->length) << " has "<< minFragments << " fragments");
      }

      // Release all of the memory regions except for the best one.
      for (uint32_t index = 0; index < attempt; ++index) {
        if (index != bestRegion) {
          memoryRegion = regionList[index];
          CIOSLOGRDMA_REQ(BGV_RDMADROP, regionList[index],
              (int)BGVRNIC_STRUCT(memoryRegion)->num_frags, _fd);
          void *buffer = regionList[index]->addr;
          size_t length = regionList[index]->length;
          ibv_dereg_mr(regionList[index]);
          ::munmap(buffer, length);
        }
      }

      // Use the best region.
      _region = regionList[bestRegion];
      memoryRegion = regionList[bestRegion];
      _frags = (int) BGVRNIC_STRUCT(memoryRegion)->num_frags;
    } else {
      // We got an unfragmented region on the first try.
      _region = regionList[0];
      _frags = (int) BGVRNIC_STRUCT(regionList[0])->num_frags;
    }
  } else {
    // a normal device does not return fragmented regions
    _region = regionList[0];
    _frags = 1;
  }

  LOG_CIOS_DEBUG_MSG("allocated memory region " << hexpointer(this) << " with local key " << getLocalKey() << " at address " << getAddress() << " with length " << hexlength(getLength()));
  CIOSLOGRDMA_REQ(BGV_RDMA_REG, _region, _frags, _fd);
  return 0;
}

/*---------------------------------------------------------------------------*/
int RdmaMemoryRegion::allocate64kB(RdmaProtectionDomainPtr pd) {
  size_t length = 65536;
  // Allocate storage for the memory region.
#ifdef RDMA_USE_MMAP
  void *buffer = ::mmap(NULL, length, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (buffer != MAP_FAILED) {
    LOG_DEBUG_MSG(
        "allocated 64kB storage for memory region with mmap OK " << length);
  }
#else
  void *buffer = malloc(length);
  if (buffer != NULL) {
    LOG_DEBUG_MSG("allocated 64kB storage for memory region with malloc OK " << length);
  }
#endif
  if (buffer == MAP_FAILED) {
    int err = errno;
    LOG_ERROR_MSG(
        "error allocating storage for memory region: "
            << RdmaError::errorString(err));
    return err;
  }

  // Register the storage as a memory region.
  int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
      | _IBV_ACCESS_REMOTE_READ;
  _region = ibv_reg_mr(pd->getDomain(), buffer, length, (ibv_access_flags) accessFlags);
  if (_region == NULL) {
    LOG_ERROR_MSG("error registering memory region");
    int err = errno;
    LOG_ERROR_MSG("ibv_reg_mr error message : " << RdmaError::errorString(err));
    ::munmap(buffer, length);
    return ENOMEM;
  }
  struct bgvrnic_mr * bgv_region = (struct bgvrnic_mr *) _region;
  _frags = (int) bgv_region->num_frags;
  //logCRdmaReg(BGV_RDMA_REG, _region->addr, _region->length, _region->lkey, _frags, _fd );
  CIOSLOGRDMA_REQ(BGV_RDMA_REG, _region, _frags, _fd);
  return 0;
}

/*---------------------------------------------------------------------------*/
int RdmaMemoryRegion::allocateFromBgvrnicDevice(RdmaProtectionDomainPtr pd,
    size_t length) {
  unsigned long region_size;

  // Open I/O memory device.
  _fd = open("/dev/bgiomem", O_RDWR);
  if (_fd == -1) {
    int err = errno;
    LOG_DEBUG_MSG(
        "error allocating file desciptor for memory region mmap: "
            << RdmaError::errorString(err));
    return err;
  }

  // Verify large region size.
  if (read(_fd, &region_size, sizeof(region_size)) == sizeof(region_size)) {
    if (length > region_size) {
      int err = EINVAL;
      close(_fd);
      LOG_WARN_MSG(
          "Allocation length of " << length << " > large region size of "
              << region_size);
      return err;
    }
  } else {
    LOG_ERROR_MSG("Unable to verify large region size.");
  }

  // Allocate storage for the memory region.
  void *buffer = ::mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd,
      0);
  if (buffer == MAP_FAILED) {
    int err = errno;
    close(_fd);
    LOG_WARN_MSG(
        "mmap allocateFromBgvrnicDevice errno=" << errno << " "
            << RdmaError::errorString(err));
    return err;
  }

  // Register the storage as a memory region.
  int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
      | _IBV_ACCESS_REMOTE_READ;
  _region = ibv_reg_mr(pd->getDomain(), buffer, length,
      (ibv_access_flags) accessFlags);
  if (_region == NULL) {
    LOG_WARN_MSG(
        "ibv_reg_mr allocateFromBgvrnicDevice error registering memory region");
    ::munmap(buffer, length);
    close(_fd);
    return ENOMEM;
  }
  struct bgvrnic_mr * bgv_region = (struct bgvrnic_mr *) _region;
  _frags = (int) bgv_region->num_frags;
  close(_fd);
  CIOSLOGRDMA_REQ(BGV_RDMA_REG, _region, _frags, _fd);
  return 0;
}

/*---------------------------------------------------------------------------*/
int RdmaMemoryRegion::release(void) {
  LOG_DEBUG_MSG("About to release memory region with local key " << getLocalKey());
  if (_region != NULL) {
    void *buffer = getAddress();
    uint32_t length = getLength();
    if (ibv_dereg_mr(_region)) {
      LOG_ERROR_MSG("Error, ibv_dereg_mr() failed\n");
      return -1;
    }
    else {
      LOG_DEBUG_MSG("released memory region with local key " << getLocalKey() << " at address " << buffer << " with length " << hexlength(length));
    }
    if (!isUserRegion()) {
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
RdmaMemoryRegion::writeTo(std::ostream& os) const {
  os << "addr=" << getAddress() << " length=" << getLength() << " lkey="
      << getLocalKey();
  os << " rkey=" << getRemoteKey() << "msglen= " << getMessageLength();
  return os;
}

