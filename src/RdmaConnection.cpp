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

#include <hpx/hpx.hpp>
#include <plugins/parcelport/verbs/rdma/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaConnection.h>
#include <plugins/parcelport/verbs/rdma/rdma_error.hpp>
#include <plugins/parcelport/verbs/rdma/event_channel.hpp>
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

#define HPX_PARCELPORT_VERBS_MAX_WORK_REQUESTS 128

using namespace hpx::parcelset::policies::verbs;
using namespace bgcios;

//LOG_DECLARE_FILE("cios.common");

RdmaConnection::RdmaConnection()
{
    // Initialize object to known state and create rdma connection management id.
    init();
    createId();
}

RdmaConnection::RdmaConnection(const std::string localAddr,
    const std::string localPort,
    const std::string remoteAddr,
    const std::string remotePort)
{
    // Initialize object to known state and create rdma connection management id.
    init();
    createId();

    // Generate the local and remote IP addresses from the input strings.
    struct sockaddr_in localAddress;
    int err = stringToAddress(localAddr, localPort, &localAddress);
    if (err != 0) {
        rdma_error e(err, "failed to generate local address");
        throw e;
    }
    struct sockaddr_in remoteAddress;
    err = stringToAddress(remoteAddr, remotePort, &remoteAddress);
    if (err != 0) {
        rdma_error e(err, "failed to generate remote address");
        throw e;
    }

    // Resolve the local and remote IP addresses to rdma addresses.
    resolveAddress(&localAddress, &remoteAddress);
}

RdmaConnection::RdmaConnection(struct rdma_cm_id *cmId,
    rdma_protection_domain_ptr domain,
    RdmaCompletionQueuePtr sendCompletionQ,
    RdmaCompletionQueuePtr recvCompletionQ,
    RdmaSharedReceiveQueuePtr SRQ,
    bool signalSendQueue)
{
    // Initialize object to known state.
    init();

    // Use the input rdma connection management id.
    _cmId = cmId;
    _srq  = SRQ;
    memcpy(&_remoteAddress, &(_cmId->route.addr.dst_addr), sizeof(struct sockaddr_in));
    // Create a queue pair.
    createQp(domain, sendCompletionQ, recvCompletionQ,
        HPX_PARCELPORT_VERBS_MAX_WORK_REQUESTS, signalSendQueue);
}

RdmaConnection::~RdmaConnection(void)
{
    // Destroy the rdma cm id and queue pair.
    if (_cmId != nullptr) {
        if (_cmId->qp != nullptr) {
            rdma_destroy_qp(_cmId); // No return code
            LOG_DEBUG_MSG(_tag << "destroyed queue pair");
        }

        if (rdma_destroy_id(_cmId) == 0) {
            LOG_DEBUG_MSG(_tag << "destroyed rdma cm id " << _cmId);
            _cmId = nullptr;
        }
        else {
            int err = errno;
            LOG_ERROR_MSG(_tag << "error destroying rdma cm id " << _cmId << ": " << rdma_error::error_string(err));
        }
    }

    // Destroy the event channel.
    if (_eventChannel != nullptr) {
        LOG_TRACE_MSG(_tag << "destroying rdma event channel with fd " << hexnumber(_eventChannel->fd));
        rdma_destroy_event_channel(_eventChannel); // No return code
        _eventChannel = nullptr;
    }

    LOG_DEBUG_MSG(_tag << "destroyed connection");
    return;
}

void
RdmaConnection::init(void)
{
    // Initialize private data.
    _eventChannel = nullptr;
    memset(&_localAddress, 0, sizeof(_localAddress));
    memset(&_remoteAddress, 0, sizeof(_remoteAddress));
    _tag = "[QP ?] ";
    _totalRecvPosted = 0;
    _totalSendPosted = 0;
    _totalReadPosted = 0;
    _totalWritePosted = 0;
    _initiated_connection = false;
    return;
}

void
RdmaConnection::createId(void)
{
    // Create the event channel.
    _eventChannel = rdma_create_event_channel();
    if (_eventChannel == nullptr) {
        rdma_error e(EINVAL, "rdma_create_event_channel() failed");
        LOG_ERROR_MSG(_tag << "error creating rdma event channel: " << rdma_error::error_string(e.error_code()));
        throw e;
    }
    LOG_DEBUG_MSG(_tag << "created rdma event channel with fd " << hexnumber(_eventChannel->fd));

    // Create the rdma cm id.
    int err = rdma_create_id(_eventChannel, &_cmId, this, RDMA_PS_TCP);
    if (err != 0) {
        rdma_error e(err, "rdma_create_id() failed");
        LOG_ERROR_MSG(_tag << "error creating rdma cm id: " << rdma_error::error_string(err));
        throw e;
    }

    LOG_DEBUG_MSG(_tag << "created rdma cm id " << _cmId);
    return;
}

void
RdmaConnection::createQp(rdma_protection_domain_ptr domain,
    RdmaCompletionQueuePtr sendCompletionQ, RdmaCompletionQueuePtr recvCompletionQ,
    uint32_t maxWorkRequests, bool signalSendQueue)
{
    // Create a queue pair.
    struct ibv_qp_init_attr qpAttributes;
    memset(&qpAttributes, 0, sizeof qpAttributes);
    qpAttributes.cap.max_send_wr = 4096; // maxWorkRequests;
    qpAttributes.cap.max_recv_wr = 4096; // maxWorkRequests;
    qpAttributes.cap.max_send_sge = 6; // 6;
    qpAttributes.cap.max_recv_sge = 6; // 6;
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
    //   int rc = (_cmId->qp==nullptr);

    if (rc != 0) {
        rdma_error e(errno, "rdma_create_qp() failed");
        LOG_ERROR_MSG(_tag << "createQp : error creating queue pair: " << hexpointer(this)
            "local address " << addressToString(&_localAddress)
            << " remote address " << addressToString(&_remoteAddress)
            << rdma_error::error_string(e.error_code()));
        throw e;
    }

    std::ostringstream tag;
    tag << "[QP " << _cmId->qp->qp_num << "] ";
    _tag = tag.str();

    LOG_DEBUG_MSG(_tag << "created queue pair " << decnumber(_cmId->qp->qp_num)
        << " max inline data is " << hexnumber(qpAttributes.cap.max_inline_data));

    return;
}

int
RdmaConnection::resolveAddress(struct sockaddr_in *localAddr, struct sockaddr_in *remoteAddr)
{
    // Resolve the addresses.
    int rc = rdma_resolve_addr(_cmId, (struct sockaddr *)localAddr, (struct sockaddr *)remoteAddr, 1000); // Configurable timeout?
    if (rc != 0) {
        rdma_error e(errno, "rdma_resolve_addr() failed");
        LOG_ERROR_MSG(_tag << "error resolving remote address " << addressToString(remoteAddr) << ": " << rdma_error::error_string(e.error_code()));
        throw e;
    }

    // Save the addresses.
    memcpy(&_remoteAddress, remoteAddr, sizeof(struct sockaddr_in));
    if (localAddr != nullptr) {
        memcpy(&_localAddress, localAddr, sizeof(struct sockaddr_in));
    }

    LOG_DEVEL_MSG("Called  rdma_resolve_addr    from "
        << ipaddress(_localAddress.sin_addr.s_addr)
        << "to " << ipaddress(_remoteAddress.sin_addr.s_addr)
        << "( " << ipaddress(_localAddress.sin_addr.s_addr) << ")");

    // Wait for RDMA_CM_EVENT_ADDR_RESOLVED event.
    struct rdma_cm_event *event;
    int err = event_channel::get_event(_eventChannel, event_channel::do_ack_event,
        RDMA_CM_EVENT_ADDR_RESOLVED, event);
    if (err != 0) {
        std::cout << _tag << "local address " << addressToString(&_localAddress)
               << " remote address " << addressToString(&_remoteAddress)
               << " " << errno << " " << rdma_error::error_string(errno)
        << std::endl;
    }

    LOG_DEBUG_MSG(_tag << "resolved to address " << addressToString(&_remoteAddress));

    return err;
}

int RdmaConnection::accept()
{
    //
    // Debugging code to get ip address of soure/dest of event
    // NB: The src and dest fields refer to the message and not the connect request
    // so we are actually receiving a request from dest (it is the src of the msg)
    //
    struct sockaddr *ip_src = &_cmId->route.addr.src_addr;
    struct sockaddr_in *addr_src = reinterpret_cast<struct sockaddr_in *>(ip_src);
    //
    struct sockaddr *ip_dst = &_cmId->route.addr.dst_addr;
    struct sockaddr_in *addr_dst = reinterpret_cast<struct sockaddr_in *>(ip_dst);
    _localAddress = *addr_src;
    _remoteAddress = *addr_dst;

    LOG_DEVEL_MSG("Calling rdma_accept          from "
        << ipaddress(_remoteAddress.sin_addr.s_addr)
        << "to " << ipaddress(_localAddress.sin_addr.s_addr)
        << "( " << ipaddress(_localAddress.sin_addr.s_addr) << ")");

    // Accept the connection request.
    struct rdma_conn_param param;
    memset(&param, 0, sizeof(param));
    param.responder_resources = 1;
    param.initiator_depth     = 1;
    param.rnr_retry_count     = 7; // special code for infinite retries

    int rc = rdma_accept(_cmId, &param);
    if (rc != 0) {
        int err = errno;
        LOG_ERROR_MSG(_tag << "error accepting connection: " << rdma_error::error_string(err));
        return err;
    }

    LOG_DEBUG_MSG(_tag << "accepted connection from client " << addressToString(&_remoteAddress));
    return 0;
}

int
RdmaConnection::reject()
{
    //
    // Debugging code to get ip address of soure/dest of event
    // NB: The src and dest fields refer to the message and not the connect request
    // so we are actually receiving a request from dest (it is the src of the msg)
    //
    struct sockaddr *ip_src = &_cmId->route.addr.src_addr;
    struct sockaddr_in *addr_src = reinterpret_cast<struct sockaddr_in *>(ip_src);
    //
    struct sockaddr *ip_dst = &_cmId->route.addr.dst_addr;
    struct sockaddr_in *addr_dst = reinterpret_cast<struct sockaddr_in *>(ip_dst);
    _localAddress = *addr_src;
    _remoteAddress = *addr_dst;

    LOG_DEVEL_MSG("Calling rdma_reject            on "
        << ipaddress(_localAddress.sin_addr.s_addr)
        << "to " << ipaddress(_remoteAddress.sin_addr.s_addr)
        << "( " << ipaddress(_localAddress.sin_addr.s_addr) << ")");

    // Reject a connection request.
    int err = rdma_reject(_cmId, 0, 0);
    if (err != 0) {
        LOG_ERROR_MSG(_tag << "error rejecting connection on cmid "
            << hexpointer(_cmId) << rdma_error::error_string(errno));
        return -1;
    }

    LOG_DEBUG_MSG(_tag << "rejected connection from new client");
    return 0;
}

int
RdmaConnection::resolveRoute(void)
{
    LOG_DEVEL_MSG("Calling rdma_resolve_route   from "
        << ipaddress(_localAddress.sin_addr.s_addr)
        << "to " << ipaddress(_remoteAddress.sin_addr.s_addr)
        << "( " << ipaddress(_localAddress.sin_addr.s_addr) << ")");
    // Resolve a route.
    int rc = rdma_resolve_route(_cmId, 1000); // Configurable timeout?
    if (rc != 0) {
        int err = errno;
        LOG_ERROR_MSG(_tag << "error resolving route: " << rdma_error::error_string(err));
        return err;
    }

    // Wait for RDMA_CM_EVENT_ROUTE_RESOLVED event.
    struct rdma_cm_event *event;
    int err = event_channel::get_event(_eventChannel, event_channel::do_ack_event,
        RDMA_CM_EVENT_ROUTE_RESOLVED, event);

    LOG_DEBUG_MSG(_tag << "resolved route to " << addressToString(&_remoteAddress));

    return err;
}

int
RdmaConnection::connect(void)
{
    LOG_DEVEL_MSG("Calling rdma_connect         from "
        << ipaddress(_localAddress.sin_addr.s_addr)
        << "to " << ipaddress(_remoteAddress.sin_addr.s_addr)
        << "( " << ipaddress(_localAddress.sin_addr.s_addr) << ")");
    // Connect to the server.
    struct rdma_conn_param param;
    memset(&param, 0, sizeof(param));
    param.responder_resources = 1;
    param.initiator_depth = 2;
    param.retry_count = 7;
    param.rnr_retry_count  = 7;
    int rc = rdma_connect(_cmId, &param);
    if (rc != 0) {
        int err = errno;
        LOG_ERROR_MSG(_tag << "error in rdma_connect to "
            << ipaddress(_remoteAddress.sin_addr.s_addr)
            << "from " << ipaddress(_localAddress.sin_addr.s_addr)
            << ": " << rdma_error::error_string(err));
        return err;
    }

    using namespace hpx::parcelset::policies::verbs;
    while (event_channel::poll_event_channel(_eventChannel->fd, [](){})==0)
    {
        //       LOG_TRACE_MSG("Do a background check in here");
        //       std::cout << "Do a background check in here" << std::endl;
    }

    // Wait for RDMA_CM_EVENT_ESTABLISHED event.
    struct rdma_cm_event *event;
    int err = event_channel::get_event(_eventChannel, event_channel::no_ack_event,
        RDMA_CM_EVENT_ESTABLISHED, event);
    if (err != 0 && event == NULL)
    {
        return err;
    }
    else if (err != 0 && event->event != RDMA_CM_EVENT_ESTABLISHED)
    {
        if (event->event == RDMA_CM_EVENT_REJECTED) {
            LOG_DEVEL_MSG("Connection rejected "
                << "from " << ipaddress(_remoteAddress.sin_addr.s_addr)
                << "( " << ipaddress(_localAddress.sin_addr.s_addr) << ")");
        } else {
            throw rdma_error(errno, "Error in connecting");
        }
        int status = event->status;
        event_channel::ack_event(event);
        //
        return status;
    }

    LOG_DEBUG_MSG(_tag << "connected to " << addressToString(&_remoteAddress));
    return event_channel::ack_event(event);
;
}

int
RdmaConnection::disconnect(bool initiate)
{
    LOG_DEBUG_MSG("RdmaConnection::disconnect");
    // Disconnect the connection.
    int err = rdma_disconnect(_cmId);
    if (err != 0) {
        err = abs(err);
        LOG_ERROR_MSG(_tag << "error disconnect: " << rdma_error::error_string(err));
        return err;
    }

    // Wait for the DISCONNECTED event if initiating the disconnect sequence.
    if (initiate) {
        LOG_INFO_MSG(_tag << "initiated disconnect");
        struct rdma_cm_event *event;
        err = event_channel::get_event(_eventChannel, event_channel::do_ack_event,
            RDMA_CM_EVENT_DISCONNECTED, event);
    }

    LOG_INFO_MSG(_tag << "disconnect completed for rdma cm id " << _cmId);
    return err;
}

uint64_t
RdmaConnection::postSendQ(struct ibv_send_wr *request)
{
    // Post the send request.
    struct ibv_send_wr *badRequest;
    LOG_TRACE_MSG(_tag << "posting "
        << wr_opcode_str(request->opcode) << " (" << request->opcode << ") work request to send queue with " << request->num_sge
        << " sge, id=" << hexpointer(request->wr_id)
        << ", imm_data=" << hexuint32(request->imm_data));
    int err = ibv_post_send(_cmId->qp, request, &badRequest);
    if (err != 0) {
        if (err==EINVAL)
        {
            rdma_error e(err, "EINVAL postSendQ");
            throw e;
        }
        else {
            LOG_ERROR_MSG(_tag << "error posting to send queue: " << rdma_error::error_string(err));
            rdma_error e(err, "posting to send queue failed");
            throw e;
        }
    }

    return request->wr_id;
}
//
// JB approved
//
uint64_t
RdmaConnection::postSend(rdma_memory_region *region, bool signaled, bool withImmediate, uint32_t immediateData)
{
    // Build scatter/gather element for outbound data.
    struct ibv_sge send_sge;
    send_sge.addr = (uint64_t)region->get_address();
    send_sge.length = region->get_message_length();
    send_sge.lkey = region->get_local_key();

    // Build a send work request.
    struct ibv_send_wr send_wr;
    memset(&send_wr, 0, sizeof(send_wr));
    send_wr.next = nullptr;
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
        << "with Length " << decnumber(send_sge.length) << hexpointer(send_sge.addr)
        << "total send posted " << decnumber(++_totalSendPosted));
    // Post a send for outbound message.
    //   ++_waitingSendPosted;
    return postSendQ(&send_wr);
}

uint64_t
RdmaConnection::postSend_xN(rdma_memory_region *region[], int N, bool signaled, bool withImmediate, uint32_t immediateData)
{
    // Build scatter/gather element for outbound data.
    struct ibv_sge send_sge[4]; // caution, don't use more than this
    int total_length = 0;
    for (int i=0; i<N; i++) {
        send_sge[i].addr   = (uint64_t)region[i]->get_address();
        send_sge[i].length = region[i]->get_message_length();
        send_sge[i].lkey   = region[i]->get_local_key();
        total_length      += send_sge[i].length;
    }

    // Build a send work request.
    struct ibv_send_wr send_wr;
    memset(&send_wr, 0, sizeof(send_wr));
    send_wr.next = nullptr;
    send_wr.sg_list = &send_sge[0];
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
    send_wr.wr_id = (uint64_t)region[0];

    LOG_TRACE_MSG(_tag << "Posted Send wr_id " << hexpointer(send_wr.wr_id) << hexpointer((uint64_t)region[1])
        << "num SGE " << decnumber(send_wr.num_sge)
        << "with Length " << decnumber(total_length) << hexpointer(send_sge[0].addr)
        << "total send posted " << decnumber(++_totalSendPosted));
    // Post a send for outbound message.
    //   ++_waitingSendPosted;
    return postSendQ(&send_wr);
}

uint64_t RdmaConnection::postSend_x0(rdma_memory_region *region, bool signaled, bool withImmediate, uint32_t immediateData)
{
    // Build scatter/gather element for outbound data.
    struct ibv_sge send_sge;
    send_sge.addr   = 0;
    send_sge.length = 0;
    send_sge.lkey   = 0;

    // Build a send work request.
    struct ibv_send_wr send_wr;
    memset(&send_wr, 0, sizeof(send_wr));
    send_wr.next = nullptr;
    send_wr.sg_list = nullptr;
    send_wr.num_sge = 0;
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

    LOG_TRACE_MSG(_tag << "Posted Zero byte Send wr_id " << hexpointer(send_wr.wr_id)
        << "with Length " << decnumber(send_sge.length)
        << "address " << hexpointer(send_sge.addr)
        << "total send posted " << decnumber(++_totalSendPosted));
    //   ++_waitingSendPosted;
    return postSendQ(&send_wr);
}

uint64_t RdmaConnection::postRead(rdma_memory_region *localregion, uint32_t remoteKey, const void *remoteAddr, std::size_t length)
{
    // Build scatter/gather element for inbound message.
    struct ibv_sge read_sge;
    read_sge.addr   = (uint64_t)localregion->get_address();
    read_sge.length = length;
    read_sge.lkey   = localregion->get_local_key();

    // Build a send work request.
    struct ibv_send_wr send_wr;
    memset(&send_wr, 0, sizeof(send_wr));
    send_wr.next                = nullptr;
    send_wr.sg_list             = &read_sge;
    send_wr.num_sge             = 1;
    send_wr.opcode              = IBV_WR_RDMA_READ;
    send_wr.send_flags          = IBV_SEND_SIGNALED; // Force completion queue to be posted with result.
    send_wr.wr_id               = (uint64_t)localregion;
    send_wr.wr.rdma.remote_addr = (uint64_t)remoteAddr;
    send_wr.wr.rdma.rkey        = remoteKey;

    // Post a send to read data.
    LOG_TRACE_MSG(_tag << "Posted Read wr_id " << hexpointer(send_wr.wr_id)
        << " with Length " << decnumber(read_sge.length) << " " << hexpointer(read_sge.addr)
        << " remote key " << decnumber(send_wr.wr.rdma.rkey) << " remote addr " << hexpointer(send_wr.wr.rdma.remote_addr));
    ++_totalReadPosted;
    return postSendQ(&send_wr);
}

int
RdmaConnection::stringToAddress(const std::string addrString, const std::string portString, struct sockaddr_in *address) const
{
    struct addrinfo *res;
    int err = getaddrinfo(addrString.c_str(), portString.c_str(), nullptr, &res);
    if (err != 0) {
        LOG_ERROR_MSG(_tag << "failed to get address info for " << addrString << ": " << rdma_error::error_string(err));
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
    if (namep != nullptr) {
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

int RdmaConnection::create_srq(rdma_protection_domain_ptr domain)
{
    try {
        _srq = std::make_shared<RdmaSharedReceiveQueue>(_cmId,domain);
    }
    catch (...) {
        return 0;
    }
    return 1;
}
