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
#include <tr1/memory>

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

   RdmaClient(struct rdma_cm_id *cmId, RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr completionQ) :
      RdmaConnection(cmId, domain, completionQ, completionQ)
   {
      createRegions(domain);
      _completionQ = completionQ;
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

   //! \brief  Get the pointer to the inbound message region.
   //! \return Pointer to beginning of region.

   void *getInboundMessagePtr(void) { return _inMessageRegion->getAddress(); }

   //! \brief  Get the pointer to the outbound message region.
   //! \return Pointer to beginning of region.

   void *getOutboundMessagePtr(void) const { return _outMessageRegion->getAddress(); }

   //! \brief  Return indicator if outbound message is ready.
   //! \return True if a message is ready, otherwise false.

   bool isOutboundMessageReady(void) const { return _outMessageRegion->isMessageReady(); }

   //! \brief  Set the message length of the outbound message region.
   //! \param  length New value for message length.
   //! \return Nothing.

   void setOutboundMessageLength(uint32_t length) { _outMessageRegion->setMessageLength(length); }

   //! \brief  Post a send operation using the outbound message region.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSendMessage(void) { return postSend(_outMessageRegion); }

   //! \brief  Post a send operation using the outbound message region with signaling enabled
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSendMsgSignaled(void) { return postSend(_outMessageRegion,true); }

   //! \brief  Post a send operation using the outbound message region.
   //! \param  length Length of message in outbound message region.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSendMessage(uint32_t length)
   { 
      setOutboundMessageLength(length);
      return postSend(_outMessageRegion);
   }

   //! \brief  Post a send operation using the outbound message region with signaling enabled
   //! \param  length Length of message in outbound message region.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSendMsgSignaled(uint32_t length)
   { 
      setOutboundMessageLength(length);
      return postSend(_outMessageRegion,true);
   }

   //! \brief  Post a receive operation using the inbound message region.
   //! \return 0 when successful, errno when unsuccessful.

   int postRecvMessage(void) { return postRecv(_inMessageRegion); }

   //! \brief  Post a receive operation using the inbound message region.
   //! \return 0 when successful, errno when unsuccessful.

   int postRecvMsg(uint64_t address) { return postRecvAddressAsID(_inMessageRegion, address, _sizeBlock); }

      //! \brief  Post a receive operation using the inbound message region.
   //! \return 0 when successful, errno when unsuccessful.

   int postRecvMsgMult(int numRecvRegions) 
  { 
    if ( _usingBlocking) return EINVAL;
    if ( (numRecvRegions * _sizeBlock) > _inMessageRegion->getLength() ) return EINVAL;
     _usingBlocking = true;
    _numRecvBlocks = numRecvRegions;
    int err = 0;
    for (int i=0;i<numRecvRegions;i++){
      err = postRecvAddressAsID(_inMessageRegion, (uint64_t)_inMessageRegion->getAddress()+(i*_sizeBlock), _sizeBlock);
      if (err) return err;
    }
    return 0;
  }

   //! \brief  Get completion queue used for both send and receive operations.
   //! \return Completion queue pointer.

   RdmaCompletionQueuePtr& getCompletionQ(void) { return _completionQ; }

   //! \brief  Get the unique id for the client.
   //! \return Unique id value.

   uint64_t getUniqueId(void) const { return _uniqueId; }

   //! \brief  Set the unique id for the client.
   //! \param  uniqueId New unique id value.
   //! \return Nothing.

   void setUniqueId(uint64_t uniqueId) { _uniqueId = uniqueId; }

   //! \brief  Create memory regions for Auxilliary functions such as from the control system to the compute nodes
   //! \param  domain Protection domain for client.
   //! \return Nothing.
   //! \throws RdmaError.

   void createRegionAuxOutbound(RdmaProtectionDomainPtr domain);

   //! \brief  Post a send operation using the Auxiliary message region relaying an inbound message
   //! \param  mh pointer to message to send using the Auxilliary message region
   //! \param  length Length of message in outbound message region.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSendAuxMessage(char *mh, uint32_t length )
   { 
      memcpy(_outMessageRegionAux->getAddress(), mh, length);
      _outMessageRegionAux->setMessageLength(length);
      return postSend(_outMessageRegionAux,false); // NOT signaled
   }


private:

   //! \brief  Create memory regions for inbound and outbound messages.
   //! \param  domain Protection domain for client.
   //! \return Nothing.
   //! \throws RdmaError.

   void createRegions(RdmaProtectionDomainPtr domain);

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

};

//! Smart pointer for RdmaClient object.
typedef std::tr1::shared_ptr<RdmaClient> RdmaClientPtr;

} // namespace bgcios

#endif // COMMON_RDMACLIENT_H

