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

//! \file  RdmaClient.h
//! \brief Declaration and inline methods for bgcios::RdmaClient class.

#ifndef COMMON_RDMACLIENT_H
#define COMMON_RDMACLIENT_H

// Includes
#include "RdmaConnection.h"
#include "RdmaMemoryRegion.h"
#include "RdmaMemoryPool.h"
#include <memory>
#include <queue>

namespace bgcios
{

//! Client for RDMA operations with a remote partner.

class RdmaClient : public RdmaConnection
{
public:

   //! \brief  Default constructor.

   RdmaClient() : RdmaConnection() {
      _numRecvBlocks = 0;
      _sizeBlock = 512;
      _usingBlocking = false;
   }

   //! \brief  Constructor.
   //! \param  localAddr Local address in dotted decimal string format.
   //! \param  localPort Local port number string.
   //! \param  remoteAddr Remote address in dotted decimal string format.
   //! \param  remotePort Remote port number string.
   //! \throws RdmaError.

   RdmaClient(const std::string localAddr, const std::string localPort, const std::string remoteAddr, const std::string remotePort) :
      RdmaConnection(localAddr, localPort, remoteAddr, remotePort)
   {
      _uniqueId = (uint64_t)-1;
      _numRecvBlocks = 0;
      _sizeBlock = 512;
      _usingBlocking = false;
   }

   //! \brief  Constructor.
   //! \param  cmId Rdma connection management id to use for new client.
   //! \param  domain Protection domain for new client.
   //! \param  completionQ Completion queue for both send and receive operations.
   //! \throws RdmaError.

   RdmaClient(struct rdma_cm_id *cmId, RdmaProtectionDomainPtr domain,
       RdmaCompletionQueuePtr completionQ, RdmaMemoryPoolPtr pool,
       RdmaSharedReceiveQueuePtr SRQ) :
      RdmaConnection(cmId, domain, completionQ, completionQ, SRQ)
   {
      createRegions(domain);
      _completionQ = completionQ;
      _memoryPool = pool;
      _uniqueId = (uint64_t)-1;
      _numRecvBlocks = 0;
      _sizeBlock = 512;
      _usingBlocking = false;
   }

   //! \brief  Default destructor.

   ~RdmaClient();

   //! \brief  Connect to a remote server.
   //! \param  domain Protection domain for new connection.
   //! \param  completionQ Completion queue for both send and receive operations.
   //! \return 0 when successful, errno when unsuccessful.

   int makePeer(RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr completionQ);

   //! \brief  Get completion queue used for both send and receive operations.
   //! \return Completion queue pointer.

   RdmaCompletionQueuePtr& getCompletionQ(void) { return _completionQ; }

   // overridden to monitor outstanding receive count
   virtual uint64_t
   postRecvRegionAsID(RdmaMemoryRegion *region, uint32_t length, bool expected=false)
   {
     uint64_t wr_id = RdmaConnection::postRecvRegionAsID(region, length, expected);
     this->pushReceive();
     return wr_id;
   }

private:

   //! \brief  Create memory regions for inbound and outbound messages.
   //! \param  domain Protection domain for client.
   //! \return Nothing.
   //! \throws RdmaError.

   void createRegions(RdmaProtectionDomainPtr domain);

   //! Memory region for inbound messages.
   RdmaProtectionDomainPtr _domain;

   //! Memory region for inbound messages.
   RdmaMemoryRegionPtr _inMessageRegion;

   //! Memory region for outbound messages.
   RdmaMemoryRegionPtr _outMessageRegion;

   //! Memory region for Auxilliary outbound messages.
   RdmaMemoryRegionPtr _outMessageRegionAux;

   //! Completion queue.
   RdmaCompletionQueuePtr _completionQ;

   //! Unique id to identify the client.
   uint64_t _uniqueId;

   //! number of receive blocks and the blocking size and whether using blocking
   int _numRecvBlocks;
   int _sizeBlock;
   bool  _usingBlocking;

   //std::shared_ptr<pinned_pool> _pinned_memory_pool;
};

//! Smart pointer for RdmaClient object.
typedef std::shared_ptr<RdmaClient> RdmaClientPtr;

} // namespace bgcios

#endif // COMMON_RDMACLIENT_H

