
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
#include "rdmahelper_logging.h"
#include <ramdisk/include/services/common/CNKMemoryRegion.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <iostream>
#include <iomanip>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>

#include <mutex>
#include <thread>

std::mutex alloc_mutex;

/*---------------------------------------------------------------------------*/
RdmaMemoryRegion::RdmaMemoryRegion(int Rdma_fd, const void *buffer, const uint64_t length)
{
  _region.address = (void*)buffer;
  _messageLength = length;
  _frags = -1; // frags = -1 is a special flag we use to tell destructor not to deallocate the memory on close
  _fd = -1;

  int success = Kernel_RDMARegisterMem(Rdma_fd, &this->_region);
  if (success!=0) {
    LOG_ERROR_MSG(
        "error registering memory region: " << length
            << " " << bgcios::errorString(success));
  }
  else {
    LOG_DEBUG_MSG(
        "OK registering memory =" << hexpointer(buffer) << " : "
        << hexpointer(_region.address) << " length " << length);
  }
}

/*---------------------------------------------------------------------------*/
int RdmaMemoryRegion::allocate(int Rdma_fd, size_t length) {
  // Obtain lock to serialize getting storage for memory regions.  This makes it more likely that we'll get contiguous physical pages
  // for the memory region (maybe someday we'll be able to use huge pages).
    void *buffer = ::mmap(NULL, length, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (buffer != MAP_FAILED) {
      LOG_DEBUG_MSG(
          "allocated storage for memory region with mmap OK, vlength = " << hexlength(length));
    } else if (buffer == MAP_FAILED) {
      int err = errno;
      LOG_ERROR_MSG("error allocating storage using mmap for memory region: " << hexlength(length)
              << " " << bgcios::errorString(err));
      return err;
    }

    this->_region.address = buffer;
    this->_region.length = length;

    LOG_DEBUG_MSG(
      "about to register memory, rdma_fd is " << Rdma_fd);
    int success = Kernel_RDMARegisterMem(Rdma_fd, &this->_region);
    if (success!=0) {
      LOG_ERROR_MSG(
          "error registering memory region: " << length
              << " " << bgcios::errorString(success));
      return success;
    }

  return 0;
}

/*---------------------------------------------------------------------------*/
int RdmaMemoryRegion::release(void) {
  /*
  if (_region != NULL) {
    CIOSLOGRDMA_REQ(BGV_RDMA_RMV, _region, _frags, _fd);
    //      LOG_CIOS_DEBUG_MSG("released memory region with local key " << getLocalKey() << " at address " << buffer << " with length " << length);
    void *buffer = getAddress();
    uint32_t length = getLength();
    cnv_dereg_mr(_region);
    // _frags == -1 is special to tell us not to release the memory, just unregister it
    if (_frags != -1) {
#ifdef RDMA_USE_MMAP
      ::munmap(buffer, length);
#else
      free(buffer);
#endif
    }
    _region = NULL;
  }
*/
  return 0;
}

/*---------------------------------------------------------------------------*/
std::ostream&
RdmaMemoryRegion::writeTo(std::ostream& os) const {
  os << "addr=" << getAddress() << " length=" << getLength() << " lkey="
      << getLocalKey();
  //os << " rkey=" << getRemoteKey()
  os << "msglen= " << getMessageLength();
  return os;
}

