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

//! \file  RdmaClient.cc
//! \brief Methods for bgcios::RdmaClient class.

#include <plugins/parcelport/verbs/rdma/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaClient.h>
#include <plugins/parcelport/verbs/rdma/rdma_error.hpp>
#include <chrono>
#include <thread>

using namespace hpx::parcelset::policies::verbs;
using namespace bgcios;

//rdma_protection_domain_ptr pinned_allocator_malloc_free::_protectionDomain;
//rdma_memory_region_ptr     pinned_allocator_malloc_free::_region;
//std::mutex              pinned_allocator_malloc_free::_pd_mutex;

//LOG_DECLARE_FILE("cios.common");
/*---------------------------------------------------------------------------*/
RdmaClient::~RdmaClient()
{
  LOG_DEBUG_MSG("Client destructor being called");
}
/*---------------------------------------------------------------------------*/
int
RdmaClient::makePeer(rdma_protection_domain_ptr domain, RdmaCompletionQueuePtr completionQ)
{
   this->_domain = domain;
   this->_completionQ = completionQ;

   // Create a queue pair.
   try {
      createQp(domain, completionQ, completionQ, 1, false);
   }
   catch (rdma_error& e) {
      LOG_ERROR_MSG(_tag << "makePeer error creating queue pair: " << rdma_error::error_string(e.error_code()));
      return e.error_code();
   }

   // Resolve a route to the server.
   int err = resolveRoute();
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error resolving route to "
          << ipaddress(_remoteAddress.sin_addr.s_addr)
          << "from " << ipaddress(_localAddress.sin_addr.s_addr)
          << ": " << rdma_error::error_string(err));
      return err;
   }

   // Connect to server.
   err = connect();
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error in makePeer connecting to "
          << ipaddress(_remoteAddress.sin_addr.s_addr)
          << "from " << ipaddress(_localAddress.sin_addr.s_addr)
          << ": " << rdma_error::error_string(err));

      LOG_ERROR_MSG(_tag << "destroy qp ");
      rdma_destroy_qp(_cmId);

      LOG_ERROR_MSG(_tag << "reset domain ");
      this->_domain.reset();

      LOG_ERROR_MSG(_tag << "reset CQ " << _completionQ->_tag);
      this->_completionQ.reset();

      return err;
   }

   return 0;
}


