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

#include "rdmahelper_logging.h"
#include <ramdisk/include/services/common/RdmaClient.h>
#include <ramdisk/include/services/common/RdmaError.h>
#include <ramdisk/include/services/MessageHeader.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <chrono>
#include <thread>

#include "ramdisk/include/services/common/RdmaRegisteredMemoryPool.h"

using namespace bgcios;

//RdmaProtectionDomainPtr pinned_allocator_malloc_free::_protectionDomain;
//RdmaMemoryRegionPtr     pinned_allocator_malloc_free::_region;
//std::mutex              pinned_allocator_malloc_free::_pd_mutex;

//LOG_DECLARE_FILE("cios.common");
/*---------------------------------------------------------------------------*/
RdmaClient::~RdmaClient()
{
  LOG_DEBUG_MSG("Client destructor being called");
   // Destroy memory region for inbound messages.
   if (_inMessageRegion != NULL) {
      LOG_CIOS_DEBUG_MSG(_tag << "destroying inbound memory region");
      _inMessageRegion.reset();
   }

   // Destroy memory region for outbound messages.
   if (_outMessageRegion != NULL) {
      LOG_CIOS_DEBUG_MSG(_tag << "destroying outbound memory region");
      _outMessageRegion.reset();
   }

   // clear memory pool reference
   LOG_CIOS_DEBUG_MSG(_tag << "releasing memory pool reference");
   _memoryPool.reset();
}
/*---------------------------------------------------------------------------*/
void
RdmaClient::createRegions(RdmaProtectionDomainPtr domain)
{
   // Create a memory region for inbound messages.
   _inMessageRegion = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
   int err = _inMessageRegion->allocate64kB(domain);
//   int err = _inMessageRegion->allocate(domain, 4096);
   if (err != 0) {
      RdmaError e(err, "allocating inbound memory region failed");
      LOG_ERROR_MSG(_tag << "HERE : error allocating inbound message region: " << bgcios::errorString(err));
      throw e;
   }

   // Create a memory region for outbound messages.
   _outMessageRegion = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
   err = _outMessageRegion->allocate64kB(domain);
   if (err != 0) {
      RdmaError e(err, "allocating outbound memory region failed");
      LOG_ERROR_MSG(_tag << "error allocating outbound message region: " << bgcios::errorString(err));
      throw e;
   }
   return;
}
/*---------------------------------------------------------------------------*/
/*
void
RdmaClient::createRegionAuxOutbound(RdmaProtectionDomainPtr domain)
{
   // Create auxilliary memory region for outbound messages.
   _outMessageRegionAux = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
   int err = _outMessageRegionAux->allocate(domain, bgcios::SmallMessageRegionSize);
   if (err != 0) {
      RdmaError e(err, "allocating outbound memory region failed");
      LOG_ERROR_MSG(_tag << "error allocating outbound message region: " << bgcios::errorString(err));
      throw e;
   }

   return;
}
*/
/*---------------------------------------------------------------------------*/
int
RdmaClient::makePeer(RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr completionQ)
{
   this->_domain = domain;
   this->_completionQ = completionQ;

   // Create memory regions.
   try {
      createRegions(domain);
   }
   catch (RdmaError& e) {
      LOG_ERROR_MSG(_tag << "error creating memory regions for messages: " << bgcios::errorString(e.errcode()));
      return e.errcode();
   }

   // Create a queue pair.
   try {
      createQp(domain, completionQ, completionQ, 1, false);
   }
   catch (RdmaError& e) {
      LOG_ERROR_MSG(_tag << "error creating queue pair: " << bgcios::errorString(e.errcode()));
      return e.errcode();
   }

   // Post a receive to get the first message.

//   postRecvMessage();

   // Resolve a route to the server.
   int err = resolveRoute();
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error resolving route to " << addressToString(&_remoteAddress) << ": " << bgcios::errorString(err));
      return err;
   }

   // Connect to server.
   err = connect();
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error connecting to " << addressToString(&_remoteAddress) << ": " << bgcios::errorString(err));
      return err;
   }

   return 0;
}

/*---------------------------------------------------------------------------*/
RdmaMemoryRegion * RdmaClient::getFreeRegion()
{
  RdmaMemoryRegion *region = this->_memoryPool->create();
  region->setMessageLength(512);

  return region;
}

