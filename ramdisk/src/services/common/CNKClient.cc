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
#include <ramdisk/include/services/common/CNKClient.h>
#include <ramdisk/include/services/common/CNKMemoryRegion.h>
#include <ramdisk/include/services/common/RdmaError.h>
//#include <ramdisk/include/services/MessageHeader.h>
//#include <ramdisk/include/services/ServicesConstants.h>
#include <chrono>
#include <thread>


using namespace bgcios;

//RdmaProtectionDomainPtr pinned_allocator_malloc_free::_protectionDomain;
//RdmaMemoryRegionPtr     pinned_allocator_malloc_free::_region;
//std::mutex              pinned_allocator_malloc_free::_pd_mutex;

//LOG_DECLARE_FILE("cios.common");
/*---------------------------------------------------------------------------*/
RdmaClient::RdmaClient(const std::string localAddr, const std::string localPort, const std::string remoteAddr, const std::string remotePort)
{
  port = boost::lexical_cast<int>(localPort);
  int success = (Kernel_RDMAOpen(&this->Rdma_FD)==0);
  if (!success) {
    throw "RDMA failed initialization";
  }
}
/*---------------------------------------------------------------------------*/
RdmaClient::~RdmaClient()
{
  LOG_DEBUG_MSG("Client destructor being called");
   // Destroy memory region for inbound messages.
   if (_inMessageRegion != NULL) {
      LOG_CIOS_DEBUG_MSG("destroying inbound memory region");
      _inMessageRegion.reset();
   }

   // Destroy memory region for outbound messages.
   if (_outMessageRegion != NULL) {
      LOG_CIOS_DEBUG_MSG("destroying outbound memory region");
      _outMessageRegion.reset();
   }

   // clear memory pool reference
   LOG_CIOS_DEBUG_MSG("releasing memory pool reference");
   _memoryPool.reset();
}
/*---------------------------------------------------------------------------*/
void
RdmaClient::createRegions()
{

  return;
 /*
   // Create a memory region for inbound messages.
   _inMessageRegion = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
   int err = _inMessageRegion->allocate64kB();
//   int err = _inMessageRegion->allocate(4096);
   if (err != 0) {
      RdmaError e(err, "allocating inbound memory region failed");
      LOG_ERROR_MSG("HERE : error allocating inbound message region: " << bgcios::errorString(err));
      throw e;
   }

   // Create a memory region for outbound messages.
   _outMessageRegion = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
   err = _outMessageRegion->allocate64kB();
   if (err != 0) {
      RdmaError e(err, "allocating outbound memory region failed");
      LOG_ERROR_MSG("error allocating outbound message region: " << bgcios::errorString(err));
      throw e;
   }
   return;
*/
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
      LOG_ERROR_MSG("error allocating outbound message region: " << bgcios::errorString(err));
      throw e;
   }

   return;
}
*/
/*---------------------------------------------------------------------------*/
int
RdmaClient::makePeer()
{
   // Create memory regions.
   try {
      createRegions();
   }
   catch (RdmaError& e) {
      LOG_ERROR_MSG("error creating memory regions for messages: " << bgcios::errorString(e.errcode()));
      return e.errcode();
   }

    int success = (Kernel_RDMAConnect(Rdma_FD, port)==0);
    if (success) {
      std::cout << "Kernel_RDMAConnect was successful using port " << std::dec << port << std::endl;
    }
    else {
      throw "Kernel_RDMAConnect failed";
    }

   // Post a receive to get the first message.
//   postRecvMessage();

   return 0;
}

/*---------------------------------------------------------------------------*/
RdmaMemoryRegionPtr RdmaClient::getFreeRegion(size_t size)
{
  RdmaMemoryRegionPtr region = this->_memoryPool->allocate(size);
  if (!region) {
    LOG_ERROR_MSG("Error creating free memory region");
  }
  region->setMessageLength(size);

  return region;
}

/*---------------------------------------------------------------------------*/
void RdmaClient::releaseRegion(RdmaMemoryRegion *region)
{
 this->_memoryPool->deallocate(region);
}
