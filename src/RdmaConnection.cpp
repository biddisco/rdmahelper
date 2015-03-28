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

//! \file  RdmaConnection.cc
//! \brief Methods for bgcios::RdmaConnection class.

#include "RdmaLogging.h"
#include <RdmaConnection.h>
#include <RdmaError.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#define CIOS_WORK_REQUESTS 128

using namespace bgcios;

//LOG_DECLARE_FILE("cios.common");

RdmaConnection::RdmaConnection()
{
   // Initialize object to known state and create rdma connection management id.
   init();
   createId();
}

RdmaConnection::RdmaConnection(const std::string localAddr, const std::string localPort, const std::string remoteAddr, const std::string remotePort)
{
   // Initialize object to known state and create rdma connection management id.
   init();
   createId();

   // Generate the local and remote IP addresses from the input strings.
   struct sockaddr_in localAddress;
   int err = stringToAddress(localAddr, localPort, &localAddress);
   if (err != 0) {
      RdmaError e(err, "failed to generate local address");
      throw e;
   }
   struct sockaddr_in remoteAddress;
   err = stringToAddress(remoteAddr, remotePort, &remoteAddress);
   if (err != 0) {
      RdmaError e(err, "failed to generate remote address");
      throw e;
   }

   // Resolve the local and remote IP addresses to rdma addresses.
   resolveAddress(&localAddress, &remoteAddress);
}

RdmaConnection::RdmaConnection(struct rdma_cm_id *cmId, RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr sendCompletionQ,
                               RdmaCompletionQueuePtr recvCompletionQ, RdmaSharedReceiveQueuePtr SRQ, bool signalSendQueue)
{
   // Initialize object to known state.
   init();

   // Use the input rdma connection management id.
   _cmId = cmId;
   _srq  = SRQ;
   memcpy(&_remoteAddress, &(_cmId->route.addr.dst_addr), sizeof(struct sockaddr_in));

   // Create a queue pair.
   createQp(domain, sendCompletionQ, recvCompletionQ, CIOS_WORK_REQUESTS, signalSendQueue);
}

RdmaConnection::~RdmaConnection(void)
{
   // Destroy the rdma cm id and queue pair.
   if (_cmId != NULL) {
      if (_cmId->qp != NULL) {
         rdma_destroy_qp(_cmId); // No return code
         LOG_CIOS_DEBUG_MSG(_tag << "destroyed queue pair");
      }
      
      if (rdma_destroy_id(_cmId) == 0) {
         LOG_CIOS_DEBUG_MSG(_tag << "destroyed rdma cm id " << _cmId);
         _cmId = NULL;
      }
      else {
         int err = errno;
         LOG_ERROR_MSG(_tag << "error destroying rdma cm id " << _cmId << ": " << RdmaError::errorString(err));
      }
   }

   // Destroy the event channel.
   if (_eventChannel != NULL) {
     LOG_CIOS_TRACE_MSG(_tag << "destroying rdma event channel with fd " << hexnumber(_eventChannel->fd));
     rdma_destroy_event_channel(_eventChannel); // No return code
     _eventChannel = NULL;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "destroyed connection");
   return;
}

void
RdmaConnection::init(void)
{
   // Initialize private data.
   _eventChannel = NULL;
   _event = NULL;
   memset(&_localAddress, 0, sizeof(_localAddress));
   memset(&_remoteAddress, 0, sizeof(_remoteAddress));
   _tag = "[QP ?] ";
   _totalRecvPosted = 0;
   _totalSendPosted = 0;
   _totalReadPosted = 0;
   _totalWritePosted = 0;
   return;
}

void
RdmaConnection::createId(void)
{
   // Create the event channel.
   _eventChannel = rdma_create_event_channel();
   if (_eventChannel == NULL) {
      RdmaError e(EINVAL, "rdma_create_event_channel() failed");
      LOG_ERROR_MSG(_tag << "error creating rdma event channel: " << RdmaError::errorString(e.errcode()));
      throw e;
   }
   LOG_CIOS_DEBUG_MSG(_tag << "created rdma event channel with fd " << hexnumber(_eventChannel->fd));

   // Create the rdma cm id.
   int err = rdma_create_id(_eventChannel, &_cmId, this, RDMA_PS_TCP);
   if (err != 0) {
      RdmaError e(err, "rdma_create_id() failed");
      LOG_ERROR_MSG(_tag << "error creating rdma cm id: " << RdmaError::errorString(err));
      throw e;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "created rdma cm id " << _cmId);
   return;
}

void
RdmaConnection::createQp(RdmaProtectionDomainPtr domain, RdmaCompletionQueuePtr sendCompletionQ, RdmaCompletionQueuePtr recvCompletionQ,
                         uint32_t maxWorkRequests, bool signalSendQueue)
{
   // Create a queue pair.
   struct ibv_qp_init_attr qpAttributes;
   memset(&qpAttributes, 0, sizeof qpAttributes);
   qpAttributes.cap.max_send_wr = 4096; // maxWorkRequests;
   qpAttributes.cap.max_recv_wr = 4096; // maxWorkRequests;
   qpAttributes.cap.max_send_sge = 1; //6;
   qpAttributes.cap.max_recv_sge = 1; // 6;
   qpAttributes.qp_context = this; // Save the pointer this object.
   qpAttributes.sq_sig_all = signalSendQueue;
   qpAttributes.qp_type = IBV_QPT_RC;
   qpAttributes.send_cq = sendCompletionQ->getQueue();
   qpAttributes.recv_cq = recvCompletionQ->getQueue();
   LOG_DEBUG_MSG("Setting SRQ to " << get_SRQ());
   qpAttributes.srq     = get_SRQ();

   int rc = rdma_create_qp(_cmId, domain->getDomain(), &qpAttributes);

   LOG_DEBUG_MSG("After Create QP, SRQ is " << get_SRQ());

//   _cmId->qp = ibv_create_qp(domain->getDomain(), &qpAttributes);
//   int rc = (_cmId->qp==NULL);

   if (rc != 0) {
      RdmaError e(errno, "rdma_create_qp() failed");
      LOG_ERROR_MSG(_tag << "error creating queue pair: " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   std::ostringstream tag;
   tag << "[QP " << _cmId->qp->qp_num << "] ";
   _tag = tag.str();

   LOG_CIOS_DEBUG_MSG(_tag << "created queue pair " << decnumber(_cmId->qp->qp_num));
   return;
}

void
RdmaConnection::resolveAddress(struct sockaddr_in *localAddr, struct sockaddr_in *remoteAddr)
{
   // Resolve the addresses.
   int rc = rdma_resolve_addr(_cmId, (struct sockaddr *)localAddr, (struct sockaddr *)remoteAddr, 1000); // Configurable timeout?
   if (rc != 0) {
      RdmaError e(errno, "rdma_resolve_addr() failed");
      LOG_ERROR_MSG(_tag << "error resolving remote address " << addressToString(remoteAddr) << ": " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   // Save the addresses.
   memcpy(&_remoteAddress, remoteAddr, sizeof(struct sockaddr_in));
   if (localAddr != NULL) {
      memcpy(&_localAddress, localAddr, sizeof(struct sockaddr_in));
   }

   // Wait for ADDR_RESOLVED event.
   int err = waitForEvent();
   if (err != 0) {
      RdmaError e(err, "waiting for ADDR_RESOVLED event failed");
      throw e;
   }
   if (_event->event != RDMA_CM_EVENT_ADDR_RESOLVED) {
      LOG_ERROR_MSG(_tag << "event " << rdma_event_str(_event->event) << " is not " << rdma_event_str(RDMA_CM_EVENT_ADDR_RESOLVED));
      RdmaError e(EINVAL);
      throw e;
   }

   // Acknowledge the ADDR_RESOLVED event.
   err = ackEvent();
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error acking " << rdma_event_str(_event->event) << ": " << RdmaError::errorString(err));
      RdmaError e(err, "acknowledge ADDR_RESOLVED event failed");
      throw e;
   }
   LOG_CIOS_DEBUG_MSG(_tag << "resolved to address " << addressToString(&_remoteAddress));

   return;
}

int
RdmaConnection::migrateId(void)
{
   int err = 0;

   // Destroy the current event channel.
   if (_eventChannel != NULL) {
      rdma_destroy_event_channel(_eventChannel);
      _eventChannel = NULL;
   }

   // Create a new event channel.
   _eventChannel = rdma_create_event_channel();
   if (_eventChannel == NULL) {
      err = errno;
      std::cout << _tag << " failed to create event channel, error " << err << std::endl;
      return err;
   }

   // Migrate the id to the new event channel.
   err = rdma_migrate_id(_cmId, _eventChannel);
   if (err != 0) {
      err = abs(err);
      std::cout << _tag << " failed to migrate id, error " << err << std::endl;
      return err;
   }

   return 0;
}

int
RdmaConnection::accept(void)
{
   // Accept the connection request.
   struct rdma_conn_param param;
   memset(&param, 0, sizeof(param));
   param.responder_resources = 1;
   param.initiator_depth = 1;
   int err = rdma_accept(_cmId, &param);
   if (err != 0) {
      err = abs(err);
      LOG_ERROR_MSG(_tag << "error accepting connection: " << RdmaError::errorString(err));
      return err;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "accepted connection from client " << addressToString(&_remoteAddress));
   return 0;
}

int
RdmaConnection::reject(void)
{
   // Reject a connection request.
   int err = rdma_reject(_cmId, NULL, 0);
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error rejecting connection from new client");
      return err;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "rejected connection from new client");
   return 0;
}

int
RdmaConnection::resolveRoute(void)
{
   // Resolve a route.
   int rc = rdma_resolve_route(_cmId, 1000); // Configurable timeout?
   if (rc != 0) {
      int err = errno;
      LOG_ERROR_MSG(_tag << "error resolving route: " << RdmaError::errorString(err));
      return err;
   }

   // Wait for ROUTE_RESOLVED event.
   int err = waitForEvent();
   if (err != 0) {
      return err;
   }
   if (_event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
      LOG_ERROR_MSG(_tag << "event " << rdma_event_str(_event->event) << " is not " << rdma_event_str(RDMA_CM_EVENT_ROUTE_RESOLVED));
      return EINVAL;
   }

   // Acknowledge the ROUTE_RESOLVED event.
   err = ackEvent();
   if (err != 0) {
      return err;
   }
   LOG_CIOS_DEBUG_MSG(_tag << "resolved route to " << addressToString(&_remoteAddress));

   return 0;
}

int
RdmaConnection::connect(void)
{
   // Connect to the server.
   struct rdma_conn_param param;
   memset(&param, 0, sizeof(param));
   param.responder_resources = 1;
   param.initiator_depth = 1;
   param.retry_count = 5;
   int rc = rdma_connect(_cmId, &param);
   if (rc != 0) {
      int err = errno;
      LOG_ERROR_MSG(_tag << "error connecting to " << addressToString(&_remoteAddress) << ": " << RdmaError::errorString(err));
      return err;
   }

   // Wait for ESTABLISHED event.
   int err = waitForEvent();
   if (err != 0) {
      return err;
   }
   if (_event->event != RDMA_CM_EVENT_ESTABLISHED) {
      LOG_ERROR_MSG(_tag << "event " << rdma_event_str(_event->event) << " is not " << rdma_event_str(RDMA_CM_EVENT_ESTABLISHED));
      /* valid REJ from consumer will always contain private data */
      if (_event->status == 28 &&
          _event->param.conn.private_data_len) {
        LOG_ERROR_MSG(_tag << "rejected IB_CME_DESTINATION_REJECT_PRIVATE_DATA ");
      } else {
        LOG_ERROR_MSG(
           "dapl_cma_active: non-consumer REJ," <<
           " reason= " << _event->status << " DST " <<
           inet_ntoa(((struct sockaddr_in *) &_cmId->route.addr.dst_addr)->sin_addr) << " "
           << ntohs(((struct sockaddr_in *)&_cmId->route.addr.dst_addr)->sin_port)
          );
      }
      return _event->status;
   }

   // Acknowledge the ESTABLISHED event.
   err = ackEvent();
   if (err != 0) {
      return err;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "connected to " << addressToString(&_remoteAddress));
   return 0;
}

int
RdmaConnection::disconnect(bool initiate)
{
LOG_DEBUG_MSG("RdmaConnection::disconnect");
   // Disconnect the connection.
   int err = rdma_disconnect(_cmId);
   if (err != 0) {
      err = abs(err);
      LOG_ERROR_MSG(_tag << "error disconnect: " << RdmaError::errorString(err));
      return err;
   }

   // Wait for the DISCONNECTED event if initiating the disconnect sequence.
   if (initiate) {
      LOG_CIOS_INFO_MSG(_tag << "initiated disconnect");
      err = waitForEvent();
      if (err != 0) {
         return err;
      }
      if (_event->event != RDMA_CM_EVENT_DISCONNECTED) {
         LOG_ERROR_MSG(_tag << " event " << rdma_event_str(_event->event) << " is not " << rdma_event_str(RDMA_CM_EVENT_DISCONNECTED));
      }

      // Acknowledge the event.
      err = ackEvent();
      if (err != 0) {
         return err;
      }
   }

   LOG_CIOS_INFO_MSG(_tag << "disconnect completed for rdma cm id " << _cmId);
   return 0;
}

uint64_t
RdmaConnection::postSendQ(struct ibv_send_wr *request)
{
   // Post the send request.
   struct ibv_send_wr *badRequest;
   LOG_TRACE_MSG(_tag << "posting " << wr_opcode_str(request->opcode)
       << " (" << request->opcode << ") work request to send queue with "
       << request->num_sge << " sge, id=" << hexpointer(request->wr_id) << ", imm_data=0x"
       << hexpointer(request->imm_data));
   int err = ibv_post_send(_cmId->qp, request, &badRequest);
   if (err != 0) {
      if (err==EINVAL)
      {
        RdmaError e(err, "EINVAL postSendQ");
        throw e;
      }
      else {
        LOG_ERROR_MSG(_tag << "error posting to send queue: " << RdmaError::errorString(err));
        RdmaError e(err, "posting to send queue failed");
        throw e;
      }
   }

   return request->wr_id;
}
//
// JB approved
//
uint64_t
RdmaConnection::postSend(RdmaMemoryRegion *region, bool signaled, bool withImmediate, uint32_t immediateData)
{
   // Build scatter/gather element for outbound data.
   struct ibv_sge send_sge;
   send_sge.addr = (uint64_t)region->getAddress();
   send_sge.length = region->getMessageLength();
   send_sge.lkey = region->getLocalKey();

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = &send_sge;
   send_wr.num_sge = 1;
   if (withImmediate) {
      send_wr.opcode = IBV_WR_SEND_WITH_IMM;
      send_wr.imm_data = immediateData;
   }
   else {
      send_wr.opcode = IBV_WR_SEND;
   }
   if (signaled) {
      send_wr.send_flags |= IBV_SEND_SIGNALED;
   }
   // use address for wr_id
   send_wr.wr_id = (uint64_t)region;

   LOG_TRACE_MSG(_tag << "Posted Send wr_id " << hexpointer(send_wr.wr_id)
       << " with Length " << decnumber(send_sge.length) << " " << hexpointer(send_sge.addr));
   // Post a send for outbound message.
   ++_totalSendPosted;
//   ++_waitingSendPosted;
   return postSendQ(&send_wr);
}

uint64_t
RdmaConnection::postSend_xN(RdmaMemoryRegion *region[], int N, bool signaled, bool withImmediate, uint32_t immediateData)
{
   // Build scatter/gather element for outbound data.
   struct ibv_sge send_sge[16]; // caution, don't use more than this
   int total_length = 0;
   for (int i=0; i<N; i++) {
     send_sge[i].addr = (uint64_t)region[i]->getAddress();
     send_sge[i].length = region[i]->getMessageLength();
     send_sge[i].lkey = region[i]->getLocalKey();
     total_length += send_sge[i].length;
   }

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = send_sge;
   send_wr.num_sge = N;
   if (withImmediate) {
      send_wr.opcode = IBV_WR_SEND_WITH_IMM;
      send_wr.imm_data = immediateData;
   }
   else {
      send_wr.opcode = IBV_WR_SEND;
   }
   if (signaled) {
      send_wr.send_flags |= IBV_SEND_SIGNALED;
   }
   // use address for wr_id
   send_wr.wr_id = (uint64_t)region;

   LOG_TRACE_MSG(_tag << "Posted Send wr_id " << hexpointer(send_wr.wr_id)
       << " num SGE " << N
       << " with Length " << decnumber(total_length) << " " << hexpointer(send_sge[0].addr) << " ...");
   // Post a send for outbound message.
   ++_totalSendPosted;
//   ++_waitingSendPosted;
   return postSendQ(&send_wr);
}

/*
uint64_t
RdmaConnection::postSend(RdmaMemoryRegionPtr region, void *address, uint32_t length, uint32_t immediateData)
{
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
   send_wr.opcode = IBV_WR_SEND_WITH_IMM;
   send_wr.imm_data = immediateData;
   send_wr.wr_id = region->getLocalKey(); // So memory region is available in work completion.

   // Post a send for outbound message.
   ++_totalSendPosted;
   return postSendQ(&send_wr);
}

uint64_t
RdmaConnection::postRdmaWrite(RdmaMemoryRegionPtr region, uint64_t remoteAddr, uint32_t remoteKey)
{
   // Build scatter/gather element for outbound data.
   struct ibv_sge write_sge;
   write_sge.addr = (uint64_t)region->getAddress();
   write_sge.length = region->getMessageLength();
   write_sge.lkey = region->getLocalKey();

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = &write_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_RDMA_WRITE;
   send_wr.send_flags = IBV_SEND_SIGNALED; // Force completion queue to be posted with result.
   send_wr.wr_id = (uint64_t)region->getLocalKey(); // So memory region is available in work completion.
   send_wr.wr.rdma.remote_addr = remoteAddr;
   send_wr.wr.rdma.rkey = remoteKey;

   // Post a send to read data.
   ++_totalWritePosted;
   return postSendQ(&send_wr);
}

uint64_t
RdmaConnection::postRdmaRead(RdmaMemoryRegionPtr region, uint64_t remoteAddr, uint32_t remoteKey)
{
   // Build scatter/gather element for inbound message.
   struct ibv_sge read_sge;
   read_sge.addr = (uint64_t)region->getAddress();
   read_sge.length = region->getMessageLength();
   read_sge.lkey = region->getLocalKey();

   // Build a send work request.
   struct ibv_send_wr send_wr;
   memset(&send_wr, 0, sizeof(send_wr));
   send_wr.next = NULL;
   send_wr.sg_list = &read_sge;
   send_wr.num_sge = 1;
   send_wr.opcode = IBV_WR_RDMA_READ;
   send_wr.send_flags = IBV_SEND_SIGNALED; // Force completion queue to be posted with result.
   send_wr.wr_id = (uint64_t)region->getLocalKey();
   send_wr.wr.rdma.remote_addr = remoteAddr;
   send_wr.wr.rdma.rkey = remoteKey;

   // Post a send to read data.
   ++_totalReadPosted;
   return postSendQ(&send_wr);
}


int
RdmaConnection::postRecv(RdmaMemoryRegionPtr region)
{
   // Build scatter/gather element for inbound message.
   struct ibv_sge recv_sge;
   recv_sge.addr = (uint64_t)region->getAddress();
   recv_sge.length = region->getLength();
   recv_sge.lkey = region->getLocalKey();

   // Build receive work request.
   struct ibv_recv_wr recv_wr;
   memset(&recv_wr, 0, sizeof(recv_wr));
   recv_wr.next = NULL;
   recv_wr.sg_list = &recv_sge;
   recv_wr.num_sge = 1;
   recv_wr.wr_id = (uint64_t)region->getLocalKey(); // So memory region is available in work completion.
printf("just got a local key of %d\n",recv_wr.wr_id);
   // Post a receive for inbound message.
   ++_totalRecvPosted;
   struct ibv_recv_wr *badRequest;
   int err = ibv_post_recv(_cmId->qp, &recv_wr, &badRequest);
   LOG_CIOS_TRACE_MSG(_tag << "posting RECV work request to recv queue with " << recv_wr.num_sge << " sge, id=" << recv_wr.wr_id << " qp " << _cmId->qp);
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error posting to recv queue: " << RdmaError::errorString(err));
      return err;
   }

   return 0;
}
*/
int
RdmaConnection::waitForEvent(void)
{
   // This operation can block if there are no pending events available.
   LOG_CIOS_TRACE_MSG(_tag << "waiting for rdma cm event on event channel with fd " << hexnumber(_eventChannel->fd) << " ...");
   //LOG_INFO_MSG_FORCED(_tag << "waiting for rdma cm event on event channel with fd " << _eventChannel->fd << " ...");
   int err = rdma_get_cm_event(_eventChannel, &_event);
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "error getting rdma cm event: " << RdmaError::errorString(err));
      return err;
   } 
   //LOG_INFO_MSG_FORCED(_tag << rdma_event_str(_event->event) << " (" << _event->event << ") is available for rdma cm id " << _event->id);
   CIOSLOGEVT_CH(BGV_RECV_EVT,_event);
   return 0;
}

int
RdmaConnection::ackEvent(void)
{
   if (_event == NULL) {
      LOG_ERROR_MSG(_tag << "no rdma cm event available to ack");
      return ENOENT;
   }

   LOG_CIOS_TRACE_MSG(_tag << "acking rdma cm event " << rdma_event_str(_event->event) << " (" << _event->event << ") for rdma cm id " << _event->id);
   int err = rdma_ack_cm_event(_event);
   if (err != 0) {
      err = abs(err);
      LOG_ERROR_MSG(_tag << "error acking event " << rdma_event_str(_event->event) << ": " << RdmaError::errorString(err));
      return err;
   }

   _event = NULL;
   return 0;
}

int
RdmaConnection::stringToAddress(const std::string addrString, const std::string portString, struct sockaddr_in *address) const
{
   struct addrinfo *res;
   int err = getaddrinfo(addrString.c_str(), portString.c_str(), NULL, &res);
   if (err != 0) {
      LOG_ERROR_MSG(_tag << "failed to get address info for " << addrString << ": " << RdmaError::errorString(err));
      return err;
   }

   if (res->ai_family != PF_INET) {
      LOG_ERROR_MSG(_tag << "address family is not PF_INET");
      err = EINVAL;
   }

   else {
      memcpy(address, res->ai_addr, sizeof(struct sockaddr_in));
      address->sin_port = (in_port_t)atoi(portString.c_str());
   }

   freeaddrinfo(res);
   return err;
}

std::string
RdmaConnection::addressToString(const struct sockaddr_in *address) const
{
   std::ostringstream addr;
   char addrbuf[INET_ADDRSTRLEN];
   inet_ntop(AF_INET, &(address->sin_addr.s_addr), addrbuf, INET_ADDRSTRLEN);
   addr << addrbuf << ":" << address->sin_port;
   return addr.str();
}

#if 0
std::string
RdmaConnection::getDeviceName(void)
{
   std::string name;
   const char *namep = ibv_get_device_name(_cmId->verbs->device);
   if (namep != NULL) {
      name = namep;
   }

   return name;
}

#endif

std::string
RdmaConnection::idToString(std::string label) const
{
   std::ostringstream os;
   os << label << ":" << std::endl;
   os << "verbs: " << std::hex << std::setfill('0') << _cmId->verbs << std::endl;
   os << "channel: " << std::hex << std::setfill('0') << _cmId->channel << std::endl;
   os << "context: " << std::hex << std::setfill('0') << _cmId->context << std::endl;
   os << "qp: " << std::hex << std::setfill('0') << _cmId->qp << std::endl;
   os << "port space: 0x" << std::hex << std::setfill('0') << _cmId->ps << std::endl;
   os << "port num: 0x" << std::hex << std::setfill('0') << (int)_cmId->port_num << std::endl;
   struct sockaddr_in *addr = (struct sockaddr_in *)&(_cmId->route.addr.src_addr);
   os << "source address: " << addressToString(addr) << " (" << std::hex << addr->sin_addr.s_addr << std::dec << ":" << addr->sin_port << ")" << std::endl;
   addr = (struct sockaddr_in *)&(_cmId->route.addr.dst_addr);
   os << "remote address: " << std::hex << addr->sin_addr.s_addr << std::dec << ":" << addr->sin_port << std::endl;

   return os.str();
}

std::string
RdmaConnection::qpToString(std::string label) const
{
   std::ostringstream os;
   struct ibv_qp_attr attr;
   struct ibv_qp_init_attr initAttr;
   std::cout << "query started" << std::endl;
   int err = ibv_query_qp(_cmId->qp, &attr, IBV_QP_STATE, &initAttr);
   while(1);
   std::cout << "query returned" << std::endl;
   if (err != 0) {
      os << _tag << " failed to query queue pair, error " << err;
      return os.str();
   }

   std::cout << "what the hell" << std::endl;
   os << label << ":" << std::endl;
   os << "context: 0x" << std::hex << std::setfill('0') << initAttr.qp_context << std::endl;
   os << "send cq: 0x" << std::hex << std::setfill('0') << initAttr.send_cq << std::endl;
   os << "recv cq: 0x" << std::hex << std::setfill('0') << initAttr.recv_cq << std::endl;
   os << "type: " << initAttr.qp_type << std::endl;
   os << "signal all sends: " << initAttr.sq_sig_all << std::endl;

   return os.str();
}

std::string
RdmaConnection::deviceToString(std::string label) const
{
   std::ostringstream os;
   struct ibv_device_attr attr;
   int err = ibv_query_device(_cmId->verbs, &attr);
   if (err != 0) {
      os << _tag << " failed to query device info, error " << err;
      return os.str();
   }

   os << label << ":" << std::endl;
   os << "firmware version: " << attr.fw_ver << std::endl;
   os << "hardware version: " << attr.hw_ver << std::endl;
   os << "node global unique id: 0x" << std::hex << attr.node_guid << std::dec << std::endl;
   os << "system image global unique id: 0x" << std::hex << attr.sys_image_guid << std::dec << std::endl;
   os << "maximum number of protection domains: " << attr.max_pd << std::endl;
   os << "maximum number of memory regions: " << attr.max_mr << std::endl;
   os << "maximum memory region size: " << attr.max_mr_size << std::endl;
   os << "maximum number of queue pairs: " << attr.max_qp << std::endl;
   os << "maximum outstanding work requests per queue: " << attr.max_qp_wr << std::endl;
   os << "maximum sge per work request: " << attr.max_sge << std::endl;
   os << "maximum number of completion queues: " << attr.max_cq << std::endl;
   os << "maximum cqe per completion queue: " << attr.max_cqe << std::endl;

   return os.str();
}

std::string
RdmaConnection::opCountersToString(void) const
{
   std::ostringstream os;
   os << "totalRecvPosted=" << _totalRecvPosted << " totalSendPosted=" << _totalSendPosted;
   os << " totalReadPosted=" << _totalReadPosted << " totalWritePosted=" << _totalWritePosted;
   return os.str();
}

std::string
RdmaConnection::wr_opcode_str(enum ibv_wr_opcode opcode) const
{
   std::string str;
   switch (opcode) {
      case IBV_WR_RDMA_WRITE: str = "IBV_WR_RDMA_WRITE"; break;
      case IBV_WR_RDMA_WRITE_WITH_IMM: str = "IBV_WR_RDMA_WRITE_WITH_IMM"; break;
      case IBV_WR_SEND: str = "IBV_WR_SEND"; break;
      case IBV_WR_SEND_WITH_IMM: str = "IBV_WR_SEND_WITH_IMM"; break;
      case IBV_WR_RDMA_READ: str = "IBV_WR_RDMA_READ"; break;
      case IBV_WR_ATOMIC_CMP_AND_SWP: str = "IBV_WR_ATOMIC_CMP_AND_SWAP"; break;
      case IBV_WR_ATOMIC_FETCH_AND_ADD: str = "IBV_WR_ATOMIC_FETCH_AND_ADD"; break;
   }

   return str;
}

int RdmaConnection::create_srq(RdmaProtectionDomainPtr domain)
{
  try {
    _srq = std::make_shared<RdmaSharedReceiveQueue>(_cmId,domain);
  }
 catch (...) {
    return 0;
  }
  return 1;
}
