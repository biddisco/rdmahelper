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

//! \file  RdmaConnection.h 
//! \brief Declaration and inline methods for bgcios::RdmaConnection class.

#ifndef COMMON_RDMACONNECTION_H
#define COMMON_RDMACONNECTION_H

// Includes
//#include <ramdisk/include/services/common/Cioslog.h>
#include "RdmaMemoryRegion.h"
#include "RdmaProtectionDomain.h"
#include "RdmaCompletionQueue.h"
#include "RdmaError.h"
#include <inttypes.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <stdexcept>
#include <string>
#include <memory>
#include <iostream>
#include <iomanip>

namespace bgcios
{

//! Connection for RDMA operations with a remote partner.

class RdmaConnection
{
public:

   //! \brief  Default constructor.

   RdmaConnection();

   //! \brief  Constructor.
   //! \param  localAddr Local address in dotted decimal string format.
   //! \param  localPort Local port number string.
   //! \param  remoteAddr Remote address in dotted decimal string format.
   //! \param  remotePort Remote port number string.
   //! \throws RdmaError.

   RdmaConnection(const std::string localAddr, const std::string localPort, const std::string remoteAddr, const std::string remotePort);

   //! \brief  Constructor.
   //! \param  cmId Rdma connection management id to use for new connection.
   //! \param  domain Protection domain for new connection.
   //! \param  sendCompletionQ Completion queue for send operations.
   //! \param  recvCompletionQ Completion queue for receive operations.
   //! \param  signalSendQueue True to generate completion queue entry for all send queue operations.
   //! \throws RdmaError.

   RdmaConnection(struct rdma_cm_id *cmId, RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr sendCompletionQ,
                  RdmaCompletionQueuePtr recvCompletionQ, bool signalSendQueue = false);

   //! \brief  Default destructor.

   ~RdmaConnection();

   //! \brief  Migrate rdma connection management id to a new rdma event channel.
   //! \return 0 when successful, errno when unsuccessful.

   int migrateId(void);

   //! \brief  Accept a connection on the rdma connection management id.
   //! \return 0 when successful, errno when unsuccessful.

   int accept(void);

   //! \brief  Reject a connection on the rdma connection management id.
   //! \return 0 when successful, errno when unsuccessful.

   int reject(void);

   //! \brief  Resolve remote address and optional local address from IP addresses to rdma addresses.
   //! \param  localAddr Local IPv4 address of this client (can be NULL).
   //! \param  remoteAddr Remote IPv4 address of server.
   //! \return Nothing.
   //! \throws RdmaError.

   void resolveAddress(struct sockaddr_in *localAddr, struct sockaddr_in *remoteAddr);

   //! \brief  Resolve a route to the server at the remote address.
   //! \return 0 when successful, errno when unsuccessful.

   int resolveRoute(void);

   //! \brief  Connect to the server at the remote address.
   //! \return 0 when successful, errno when unsuccessful.

   int connect(void);

   //! \brief  Disconnect the connection.
   //! \param  initiate True if initiating disconnect sequence, false if responding to disconnect from peer.
   //! \return 0 when successful, errno when unsuccessful.

   int disconnect(bool initiate);

   //! \brief  Wait for an event on RDMA event channel.
   //! \return 0 when successful, errno when unsuccessful.

   int waitForEvent(void);

   //! \brief  Acknowledge the current event on rdma event channel.
   //! \return 0 when successful, errno when unsuccessful.

   int ackEvent(void);

   //! \brief  Get the descriptor representing the rdma event channel.
   //! \return Event channel descriptor.

   int getEventChannelFd(void) const { return _eventChannel->fd; }

   //! \brief  Get the event type from the current rdma event.
   //! \return Event type value.

   rdma_cm_event_type getEventType(void) const { return _event->event; }

   //! \brief  Get the rdma connection management id from the current rdma event.
   //! \return Pointer to rdma connecton management id.

   struct rdma_cm_id *getEventId(void) const { return _event->id; }

   //! \brief  Get the queue pair number from the current rdma event.
   //! \return Queue pair number.

   uint32_t getEventQpNum(void) const { return _event->id->qp->qp_num; }

   //! \brief  Get the InfiniBand device context from the current rdma event.
   //! \return Pointer to InfiniBand device context structure.

   struct ibv_context *getEventContext(void) const { return _event->id->verbs; }

   //! \brief  Post a send operation using the specified memory region.
   //! \param  region Memory region that contains data to send.
   //! \param  signaled True to set signaled flag so a completion queue entry is generated.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSend(RdmaMemoryRegionPtr region, bool signaled, bool withImmediate, uint32_t immediateData);

   //! \brief  Post a rdma read operation from a remote memory region to the specified memory region.
   //! \param  reqID is the request ID for the requested operation
   //! \param  remoteKey Key of remote memory region.
   //! \param  remoteAddr Address of remote memory region.
   //! \param  localKey Key of local memory region.
   //! \param  localAddr Address of local memory region
   //! \param  length is the size of the transfer
   //! \return error status for the posted operation.
   //!

int
postRdmaRead(uint64_t reqID, uint32_t remoteKey, uint64_t remoteAddr,
                                             uint32_t localKey,  uint64_t localAddr,
                                             ssize_t length)
{
   // Build scatter/gather element for inbound message.
   struct ibv_send_wr *badRequest;
   struct ibv_sge read_sge;
   read_sge.addr = localAddr;
   read_sge.length = length;
   read_sge.lkey = localKey;

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = &read_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_RDMA_READ;
   send_wr.send_flags = IBV_SEND_SIGNALED; // Force completion queue to be posted with result.
   send_wr.wr_id = reqID;
   send_wr.wr.rdma.remote_addr = remoteAddr;
   send_wr.wr.rdma.rkey = remoteKey;

   // Post a send to read data.
   ++_totalReadPosted;
    int err = ibv_post_send(_cmId->qp, &send_wr, &badRequest);
   LOG_CIOS_TRACE_MSG(_tag.c_str() << "posting Read wr_id " << send_wr.wr_id << " with Length " << length << " " << std::setw(8) << std::setfill('0') << std::hex << remoteAddr);
   return err;
}

   //! \brief  Post a rdma write operation to a remote memory region from the specified memory region.
   //! \param  reqID is the request ID for the requested operation
   //! \param  remoteKey Key of remote memory region.
   //! \param  remoteAddr Address of remote memory region.
   //! \param  localKey Key of local memory region.
   //! \param  localAddr Address of local memory region
   //! \param  length is the size of the transfer
   //! \param  flags are 0 or IBV_SEND_SIGNALED typically
   //! \return error status for the posted operation.
   //!
int
postRdmaWrite(uint64_t reqID, uint32_t remoteKey, uint64_t remoteAddr,
                                             uint32_t localKey,  uint64_t localAddr,
                                             ssize_t length, int flags)
{
   // Build scatter/gather element for inbound message.
   struct ibv_send_wr *badRequest;
   struct ibv_sge read_sge;
   read_sge.addr = localAddr;
   read_sge.length = length;
   read_sge.lkey = localKey;

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = &read_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_RDMA_WRITE;
   send_wr.send_flags = flags; //if IBV_SEND_SIGNALED, Force completion queue to be posted with result.
   send_wr.wr_id = reqID;
   send_wr.wr.rdma.remote_addr = remoteAddr;
   send_wr.wr.rdma.rkey = remoteKey;

   // Post a send to read data.
   ++_totalReadPosted;
   int err = ibv_post_send(_cmId->qp, &send_wr, &badRequest);
   LOG_CIOS_TRACE_MSG(_tag.c_str() << "posting Write wr_id " << send_wr.wr_id << " with Length " << length << " " << std::setw(8) << std::setfill('0') << std::hex << remoteAddr);
   return err;
}

uint64_t
postRecvRegionAsID(RdmaMemoryRegionPtr region, uint64_t address, uint32_t length)
{
   // Build scatter/gather element for inbound message.
   struct ibv_sge recv_sge;
   recv_sge.addr =   address;
   recv_sge.length = length;
   recv_sge.lkey = region->getLocalKey();

   // Build receive work request.
   struct ibv_recv_wr recv_wr;
   memset(&recv_wr, 0, sizeof(recv_wr));
   recv_wr.next = NULL;
   recv_wr.sg_list = &recv_sge;
   recv_wr.num_sge = 1;
   recv_wr.wr_id = (uint64_t)region.get();
   ++_totalRecvPosted;
   struct ibv_recv_wr *badRequest;
   int err = ibv_post_recv(_cmId->qp, &recv_wr, &badRequest);
   if (err!=0) {
     throw(RdmaError(err, "postSendNoImmed failed"));
   }
   std::cout << "posting Recv wr_id " << recv_wr.wr_id << " with Length " << length << " " << std::setw(8) << std::setfill('0') << std::hex << address << std::endl;
   _waitingRecvPosted++;
   LOG_DEBUG_MSG(_tag.c_str() << "posting Recv wr_id " << recv_wr.wr_id << " with Length " << length << " " << std::setw(8) << std::setfill('0') << std::hex << address);
   return recv_wr.wr_id;
}

/*
   //! \brief  Post a send with immediate data operation using the specified memory region.
   //! \param  region Memory region that contains data to send.
   //! \param  immediateData Immediate data value.
   //! \param  signaled True to set signaled flag so a completion queue entry is generated.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSend(RdmaMemoryRegionPtr region, uint32_t immediateData, bool signaled = false)
   {
      return postSend(region, signaled, true, immediateData);
   }

   //! \brief  Post a send with immediate data operation from the specified address in the specified memory region.
   //! \param  region Memory region that contains data to send.
   //! \param  address Address of data in the memory region.
   //! \param  length Length of data to send.
   //! \param  immediateData Immediate data value.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postSend(RdmaMemoryRegionPtr region, void *address, uint32_t length, uint32_t immediateData);

uint64_t postSend(uint32_t regionLocalKey, void *address, uint32_t length, uint64_t refWorkId, uint32_t immediateData)
{
   struct ibv_send_wr *badRequest;
   // Build scatter/gather element for outbound data.
   struct ibv_sge send_sge;
   send_sge.addr = (uint64_t)address;
   send_sge.length = length;
   send_sge.lkey = regionLocalKey; //region->getLocalKey();
   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   //send_wr.next = NULL;
   send_wr.sg_list = &send_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_SEND_WITH_IMM;
   send_wr.imm_data = immediateData;
   send_wr.wr_id = refWorkId;   // So memory region is available in work completion.
   // Post a send for outbound message.
   ++_totalSendPosted;
   int err = ibv_post_send(_cmId->qp, &send_wr, &badRequest);
   if (err!=0) {
     throw(RdmaError(err, "postSend failed"));
   }
   return send_wr.wr_id;
}

uint64_t
postSendNoImmed(RdmaMemoryRegionPtr region, void *address, uint64_t length)
{
   struct ibv_send_wr *badRequest;
   // Build scatter/gather element for outbound data.
   struct ibv_sge send_sge;
   send_sge.addr = (uint64_t)address;
   send_sge.length = length;
   send_sge.lkey = region->getLocalKey();

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = &send_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_SEND;
   send_wr.wr_id = region->getLocalKey(); // So memory region is available in work completion.
   send_wr.send_flags = IBV_SEND_FENCE;//Always wait for the preceding operation (like in RDMA putdata to compute node not signaled)

   // Post a send for outbound message.
   ++_totalSendPosted;
   int err = ibv_post_send(_cmId->qp, &send_wr, &badRequest);
   if (err!=0) {
     throw(RdmaError(err, "postSendNoImmed failed"));
   }
   return send_wr.wr_id;
}

   //! \brief  Post a rdma write operation from the specified memory region to a remote memory region.
   //! \param  region Memory region that contains data to send.
   //! \param  remoteAddr Address of remote memory region.
   //! \param  remoteKey Key of remote memory region.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postRdmaWrite(RdmaMemoryRegionPtr region, uint64_t remoteAddr, uint32_t remoteKey);

   //! \brief  Post a rdma read operation from a remote memory region to the specified memory region.
   //! \param  region Memory region to put received data.
   //! \param  remoteAddr Address of remote memory region.
   //! \param  remoteKey Key of remote memory region.
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

   uint64_t postRdmaRead(RdmaMemoryRegionPtr region, uint64_t remoteAddr, uint32_t remoteKey);
*/

/*
int
postRdmaWriteWithAck(uint64_t reqID, uint32_t remoteKey, uint64_t remoteAddr, 
                                             uint32_t localKey,  uint64_t localAddr,
                                             ssize_t length, 
                                             RdmaMemoryRegionPtr regionSend, void *addressSend, uint64_t lengthSend)
{
   // Build scatter/gather element for inbound message.
   struct ibv_send_wr *badRequest;
   struct ibv_sge read_sge;
   read_sge.addr = localAddr;
   read_sge.length = length;
   read_sge.lkey = localKey;

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   
   send_wr.sg_list = &read_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_RDMA_WRITE;
   //ssend_wr.send_flags = 0; //No completion necessary
   send_wr.wr_id = reqID;
   send_wr.wr.rdma.remote_addr = remoteAddr;
   send_wr.wr.rdma.rkey = remoteKey;

   //append an IBV_WR_SEND
   struct ibv_send_wr send_wr2;
   send_wr.next = &send_wr2;

   struct ibv_sge send_sge;
   send_sge.addr = (uint64_t)addressSend;
   send_sge.length = lengthSend;
   send_sge.lkey = regionSend->getLocalKey();

   // Build the send work request.

   memset(&send_wr2, 0, sizeof(send_wr2));
   send_wr2.next = NULL;
   send_wr2.sg_list = &send_sge;
   send_wr2.num_sge = 1;
   send_wr2.opcode = IBV_WR_SEND;
   send_wr2.wr_id = regionSend->getLocalKey(); // So memory region is available in work completion.
   send_wr2.send_flags = IBV_SEND_FENCE;//Always wait for the preceding operation (like in RDMA putdata to compute node not signaled)
   
   int err = ibv_post_send(_cmId->qp, &send_wr, &badRequest);
   CIOSLOGPOSTSEND(BGV_POST_WRR,send_wr,err);
   CIOSLOGPOSTSEND(BGV_POST_SND,send_wr2,err);
   return err;
}

   //! \brief  Post a receive operation using the specified memory region with address specified in the region
   //! \param  region Memory region to put received data.
   //! \param  address is the Address in the local memory region
   //! \param  length is the length of the memo
   //! \return Work request id for the posted operation.
   //! \throws RdmaError.

  uint64_t
  postRecvAddressAsID(RdmaMemoryRegionPtr region, uint64_t address, uint32_t length)
  {
     // Build scatter/gather element for inbound message.
     struct ibv_sge recv_sge;
     recv_sge.addr =   address;
     recv_sge.length = length;
     recv_sge.lkey = region->getLocalKey();

     // Build receive work request.
     struct ibv_recv_wr recv_wr;
     memset(&recv_wr, 0, sizeof(recv_wr));
     recv_wr.next = NULL;
     recv_wr.sg_list = &recv_sge;
     recv_wr.num_sge = 1;
     recv_wr.wr_id = address;
     ++_totalRecvPosted;
     struct ibv_recv_wr *badRequest;
     int err = ibv_post_recv(_cmId->qp, &recv_wr, &badRequest);
     if (err!=0) {
       throw(RdmaError(err, "postSendNoImmed failed"));
     }
     return recv_wr.wr_id;
  }
*/
/*
   //! \brief  Post a receive operation using the specified memory region.
   //! \param  region Memory region to put received data.
   //! \return 0 when successful, errno when unsuccessful.

   int postRecv(RdmaMemoryRegionPtr region);
*/
   //! \brief  Decrease waiting receive counter
   //! \return the value of the waiting receive counter after decrement
  uint32_t decrementWaitingRecv() { return --_waitingRecvPosted; }
  uint32_t decrementWaitingSend() { return --_waitingSendPosted; }

   //! \brief  Get the waiting receive counter
   //! \return the value of the waiting receive counter after decrement
   uint32_t getNumWaitingRecv() { return _waitingRecvPosted; }
   uint32_t getNumWaitingSend() { return _waitingSendPosted; }

   //! \brief  Get the rdma connection management identifier for the connection.
   //! \return Pointer to rdma cm identifier structure.

   struct rdma_cm_id * getCmId(void) const { return _cmId; }

   //! \brief  Get the queue pair number for the connection.
   //! \return Queue pair number.

   uint32_t getQpNum(void) const { return _cmId->qp->qp_num; }

   //! \brief  Get the InfiniBand device context for the connection.
   //! \return Pointer to InfiniBand device context structure.

   struct ibv_context *getContext(void) const { return _cmId->verbs; }

   //! \brief  Get the local IPv4 address for the connection.
   //! \return IPv4 address.

   in_addr_t getLocalIPv4Address(void) const { return _localAddress.sin_addr.s_addr; }

   //! \brief  Get the local port for the connection.
   //! \return Port number.

   in_port_t getLocalPort(void) { 
      if (_localAddress.sin_port==0){
         _localAddress.sin_port=rdma_get_src_port(_cmId);
      }
      return _localAddress.sin_port;
   }

   //! \brief  Get the remote IPv4 address for the connection.
   //! \return IPv4 address.

   in_addr_t getRemoteIPv4Address(void) const { return _remoteAddress.sin_addr.s_addr; }

   //! \brief  Get the remote port for the connection.
   //! \return Port number.

   in_port_t getRemotePort(void) const { return _remoteAddress.sin_port; }

   //! \brief  Get the local address as a printable string.
   //! \return Address string.

   std::string getLocalAddressString(void) const { return addressToString(&_localAddress); }

   //! \brief  Get the remote address as a printable string.
   //! \return Address string.

   std::string getRemoteAddressString(void) const { return addressToString(&_remoteAddress); }

   //! \brief  Get the tag to uniquely identify trace points.
   //! \return Tag string.

   std::string& getTag(void) { return _tag; }

   //! \brief  Set the tag to uniquely identify trace points.
   //! \param  tag New value for tag string.
   //! \return Nothing.

   void setTag(std::string tag) { _tag = tag; }

   //! \brief  Convert dotted decimal string to AF_INET address.
   //! \param  addrString IP address in dotted decimal string format.
   //! \param  portString Port number string.
   //! \param  address Pointer to AF_INET address structure.
   //! \return 0 when successful, errno when unsuccessful.

   int stringToAddress(const std::string addrString, const std::string portString, struct sockaddr_in *address) const;

   //! \brief  Convert an AF_INET address to a string.
   //! \param  address Pointer to AF_INET address structure.
   //! \return Address string.

   std::string addressToString(const struct sockaddr_in *address) const;

   //! \brief  Print info about the rdma connection management id to a string.
   //! \param  label String to label output.
   //! \return Output string.

   std::string idToString(std::string label) const;

   //! \brief  Print info about the queue pair to a string.
   //! \param  label String to label output.
   //! \return Output string.

   std::string qpToString(std::string label) const;

   //! \brief  Print info about the InfiniBand device to a string.
   //! \param  label String to label output.
   //! \return Output string.

   std::string deviceToString(std::string label) const;

   //! \brief  Print queue pair operation counters to a string.
   //! \return Output string.

   std::string opCountersToString(void) const;

   //! \brief  Return a string naming a ibv_wr_opcode value.
   //! \param  opcode ibv_wr_opcode value.
   //! \return String representing value.

   std::string wr_opcode_str(enum ibv_wr_opcode opcode) const;

protected:

   //! \brief  Initialize object to known state.
   //! \return Nothing.

   void init(void);

   //! \brief  Create rdma event channel and rdma connection management id.
   //! \return Nothing.
   //! \throws RdmaError.

   void createId(void);

   //! \brief  Create queue pair for connection.
   //! \param  domain Protection domain for connection.
   //! \param  sendCompletionQ Completion queue for send operations.
   //! \param  recvCompletionQ Completion queue for receive operations.
   //! \param  maxWorkRequests Maximum number of work requests that can be posted to queue pair.
   //! \param  signalSendQueue True to generate completion queue entry for all send queue operations.
   //! \return Nothing.
   //! \throws RdmaError.

   void createQp(RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr sendCompletionQ, RdmaCompletionQueuePtr recvCompletionQ,
                 uint32_t maxWorkRequests, bool signalSendQueue);

   //! \brief  Destroy resources owned by object.
   //! \return Nothing.
   void destroy(void);

   //! \brief  Post a work request to the send queue.
   //! \param  request Pointer to send work request.
   //! \return Work request identifier.
   //! \throws RdmaError.

   uint64_t postSendQ(struct ibv_send_wr *request);

   //! Tag to identify this connection in trace points.
   std::string _tag;

   //! RDMA connection management id.
   struct rdma_cm_id *_cmId;

   //! Event channel for notification of RDMA connection management events.
   struct rdma_event_channel *_eventChannel;

   //! Current RDMA connection management event.
   struct rdma_cm_event *_event;

   //! Address of this (local) side of the connection.
   struct sockaddr_in _localAddress;

   //! Address of other (remote) side of the connection.
   struct sockaddr_in _remoteAddress;

   //! Total number of receive operations posted to queue pair.
   uint64_t _totalRecvPosted;

   //! Total number of send operations posted to queue pair.
   uint64_t _totalSendPosted;

   //! Total number of rdma read operations posted to queue pair.
   uint64_t _totalReadPosted;

   //! Total number of rdma write operations posted to queue pair.
   uint64_t _totalWritePosted;

   //! The number of outstanding receives in the queue
   uint32_t _waitingRecvPosted;
   uint32_t _waitingSendPosted;

};

//! Smart pointer for RdmaConnection object.
  typedef std::shared_ptr<RdmaConnection> RdmaConnectionPtr;

} // namespace bgcios

#endif // COMMON_RDMACONNECTION_H
