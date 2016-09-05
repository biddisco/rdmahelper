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

//! \file  rdma_protection_domain.cc
//! \brief Methods for bgcios::rdma_protection_domain class.

// Includes
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_protection_domain.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_error.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
//
#include <errno.h>

using namespace hpx::parcelset::policies::verbs;
using namespace bgcios;

//LOG_DECLARE_FILE("cios.common");


rdma_protection_domain::rdma_protection_domain(struct ibv_context *context)
{
   // Validate context pointer (since ibv_ functions won't check it).
   if (context == NULL) {
      rdma_error e(EFAULT, "device context pointer is null");
      LOG_ERROR_MSG("error with context pointer " << context << " when constructing protection domain");
      throw e;
   }

   // Allocate a protection domain.
   _protectionDomain = ibv_alloc_pd(context);
   if (_protectionDomain == NULL) {
      rdma_error e(ENOMEM, "ibv_alloc_pd() failed");
      LOG_ERROR_MSG("error allocating protection domain");
      throw e;
   }
   LOG_DEBUG_MSG("allocated protection domain " << _protectionDomain->handle);
}

rdma_protection_domain::~rdma_protection_domain()
{
   if (_protectionDomain != NULL) {
      uint32_t handle = _protectionDomain->handle;
      int err = ibv_dealloc_pd(_protectionDomain);
      if (err == 0) {
         _protectionDomain = NULL;
         LOG_DEBUG_MSG("deallocated protection domain " << handle);
      }
      else {
         LOG_ERROR_MSG("error deallocating protection domain " << handle << ": " <<  rdma_error::error_string(err));
      }
   }
}

std::ostream&
rdma_protection_domain::writeTo(std::ostream& os) const
{
   os << "context=" << _protectionDomain->context << " handle=" << _protectionDomain->handle;
   return os;
}

