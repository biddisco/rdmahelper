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

//! \file  rdma_memory_region.cc
//! \brief Methods for bgcios::rdma_memory_region class.

#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_error.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_memory_region.hpp>
//
#include <memory>
//#include <string>
//#include <fcntl.h>
//#include <bitset>

using namespace hpx::parcelset::policies::verbs;
using namespace bgcios;

/*---------------------------------------------------------------------------*/
rdma_memory_region::rdma_memory_region(rdma_protection_domainPtr pd,
    const void *buffer, const uint64_t length)
{
  _messageLength = length;
  _flags  =  BLOCK_USER;

  int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
      | _IBV_ACCESS_REMOTE_READ;

  _region = ibv_reg_mr(pd->getDomain(), (void*) buffer, _messageLength,
      (ibv_access_flags) accessFlags);
  if (_region == NULL) {
    int err = errno;
    LOG_ERROR_MSG(
        "error registering user mem ibv_reg_mr " << hexpointer(buffer) << " " << hexlength(length) << " error/message: " << err << "/"
            << rdma_error::error_string(err));
  } else {
    LOG_DEBUG_MSG(
        "OK registering memory =" << hexpointer(buffer) << " : "
        << hexpointer(_region->addr) << " length " << hexlength(length));
  }
}

/*---------------------------------------------------------------------------*/
int rdma_memory_region::allocate(rdma_protection_domainPtr pd, size_t length) {

  // Allocate memory regions until we get one that is not fragmented or there are too many attempts.
  struct ibv_mr *memoryRegion;

    // Allocate storage for the memory region.
    void *buffer = malloc(length);
    if (buffer != NULL) {
      LOG_DEBUG_MSG("allocated storage for memory region with malloc OK " << hexnumber(length));
    }

    // Register the storage as a memory region.
    int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
        | _IBV_ACCESS_REMOTE_READ;
    _region = ibv_reg_mr(pd->getDomain(), buffer, length,
        (ibv_access_flags) accessFlags);

    if (_region == NULL) {
      int err = errno;
      LOG_ERROR_MSG(
          "error registering ibv_reg_mr error message : "
              << " " << err << " " << rdma_error::error_string(err));
      return ENOMEM;
    } else {
      LOG_DEBUG_MSG(
          "OK registering ibv_reg_mr with flags : " << " " << hexnumber(accessFlags));
    }

  LOG_CIOS_DEBUG_MSG("allocated memory region " << hexpointer(this) << " with local key " << getLocalKey() << " at address " << getAddress() << " with length " << hexlength(getLength()));
//  CIOSLOGRDMA_REQ(BGV_RDMA_REG, _region, _frags, _fd);
  return 0;
}

/*---------------------------------------------------------------------------*/
int rdma_memory_region::allocate64kB(rdma_protection_domainPtr pd) {
  size_t length = 65536;
  // Allocate storage for the memory region.
  void *buffer = malloc(length);
  if (buffer != NULL) {
    LOG_DEBUG_MSG("allocated 64kB storage for memory region with malloc OK " << length);
  }

  // Register the storage as a memory region.
  int accessFlags = _IBV_ACCESS_LOCAL_WRITE | _IBV_ACCESS_REMOTE_WRITE
      | _IBV_ACCESS_REMOTE_READ;
  _region = ibv_reg_mr(pd->getDomain(), buffer, length, (ibv_access_flags) accessFlags);
  if (_region == NULL) {
    LOG_ERROR_MSG("error registering memory region");
    int err = errno;
    LOG_ERROR_MSG("ibv_reg_mr error message : " << rdma_error::error_string(err));
    free(buffer);
    return ENOMEM;
  }
  return 0;
}


/*---------------------------------------------------------------------------*/
int rdma_memory_region::release(void) {
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
      free(buffer);
    }
    _region = NULL;
  }

  return 0;
}

/*---------------------------------------------------------------------------*/

