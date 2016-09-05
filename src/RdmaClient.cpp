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

#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaClient.h>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_error.hpp>
#include <chrono>
#include <thread>

using namespace hpx::parcelset::policies::verbs;
using namespace bgcios;

//rdma_protection_domainPtr pinned_allocator_malloc_free::_protectionDomain;
//rdma_memory_region_ptr     pinned_allocator_malloc_free::_region;
//std::mutex              pinned_allocator_malloc_free::_pd_mutex;

//LOG_DECLARE_FILE("cios.common");
/*---------------------------------------------------------------------------*/
RdmaClient::~RdmaClient()
{
  LOG_DEBUG_MSG("Client destructor being called");
   // Destroy memory region for inbound messages.
   if (_inMessageRegion != NULL) {
      LOG_DEBUG_MSG(_tag << "destroying inbound memory region");
      _inMessageRegion.reset();
   }

   // Destroy memory region for outbound messages.
   if (_outMessageRegion != NULL) {
      LOG_DEBUG_MSG(_tag << "destroying outbound memory region");
      _outMessageRegion.reset();
   }
}
/*---------------------------------------------------------------------------*/
void
RdmaClient::createRegions(rdma_protection_domainPtr domain)
{
   // Create a memory region for inbound messages.
   _inMessageRegion = rdma_memory_region_ptr(new rdma_memory_region());
   int err = _inMessageRegion->allocate64kB(domain);
//   int err = _inMessageRegion->allocate(domain, 4096);
   if (err != 0) {
      rdma_error e(err, "allocating inbound memory region failed");
      LOG_ERROR_MSG(_tag << "HERE : error allocating inbound message region: " << rdma_error::error_string(err));
      throw e;
   }

   // Create a memory region for outbound messages.
   _outMessageRegion = rdma_memory_region_ptr(new rdma_memory_region());
   err = _outMessageRegion->allocate64kB(domain);
   if (err != 0) {
      rdma_error e(err, "allocating outbound memory region failed");
      LOG_ERROR_MSG(_tag << "error allocating outbound message region: " << rdma_error::error_string(err));
      throw e;
   }
   return;
}
/*---------------------------------------------------------------------------*/
/*
void
RdmaClient::createRegionAuxOutbound(rdma_protection_domainPtr domain)
{
   // Create auxilliary memory region for outbound messages.
   _outMessageRegionAux = rdma_memory_region_ptr(new rdma_memory_region());
   int err = _outMessageRegionAux->allocate(domain, bgcios::SmallMessageRegionSize);
   if (err != 0) {
      rdma_error e(err, "allocating outbound memory region failed");
      LOG_ERROR_MSG(_tag << "error allocating outbound message region: " << rdma_error::error_string(err));
      throw e;
   }

   return;
}
*/
/*---------------------------------------------------------------------------*/
int
RdmaClient::makePeer(rdma_protection_domainPtr domain, RdmaCompletionQueuePtr completionQ)
{
   this->_domain = domain;
   this->_completionQ = completionQ;

   // Create memory regions.
   try {
      createRegions(domain);
   }
   catch (rdma_error& e) {
      LOG_ERROR_MSG(_tag << "error creating memory regions for messages: " << rdma_error::error_string(e.errcode()));
      return e.errcode();
   }

   // Create a queue pair.
   try {
      createQp(domain, completionQ, completionQ, 1, false);
   }
   catch (rdma_error& e) {
      LOG_ERROR_MSG(_tag << "makePeer error creating queue pair: " << rdma_error::error_string(e.errcode()));
      return e.errcode();
   }

   // Post a receive to get the first message.

//   postRecvMessage();

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
      // delete regions
      _inMessageRegion.reset();
      _outMessageRegion.reset();

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


