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

//! \file  RdmaController.cc
//! \brief Methods for bgcios::stdio::RdmaController class.
#include "../include/RdmaController.h"

#include <RdmaError.h>
#include <RdmaDevice.h>
#include <RdmaCompletionQueue.h>
//
#include <boost/lexical_cast.hpp>
//
#include <poll.h>
#include <errno.h>
#include <iomanip>
#include <sstream>
#include <queue>
#include <stdio.h>
#include <thread>
#include <fstream>

// network stuff, only need inet_ntoa
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rdma_messages.h"
#include <plugins/parcelport/verbs/rdmahandler/event_channel.hpp>

namespace hpx {
namespace parcelset {
namespace policies {
namespace verbs
{
    event_channel::mutex_type event_channel::_event_mutex;
}}}}

using namespace bgcios;

const uint64_t LargeRegionSize = 8192;
/*---------------------------------------------------------------------------*/
RdmaController::RdmaController(const char *device, const char *interface, int port) {
  this->_device    = device;
  this->_interface = interface;
  this->_port      = port;
  this->_localAddr = 0xFFFFFFFF;
  event_poll_count = 0;
}

/*---------------------------------------------------------------------------*/
RdmaController::~RdmaController() {
  LOG_DEBUG_MSG("RdmaController destructor clearing clients");
  _clients.clear();
  LOG_DEBUG_MSG("RdmaController destructor closing server");
  this->_rdmaListener.reset();
  LOG_DEBUG_MSG("RdmaController destructor freeing regions");
  this->_largeRegion.reset();
  LOG_DEBUG_MSG("RdmaController destructor freeing memory pool");
  this->_memoryPool.reset();
  LOG_DEBUG_MSG("RdmaController destructor releasing protection domain");
  this->_protectionDomain.reset();
  LOG_DEBUG_MSG("RdmaController destructor deleting completion channel");
  this->_completionChannel.reset();
  LOG_DEBUG_MSG("RdmaController destructor done");
}

/*---------------------------------------------------------------------------*/
int RdmaController::startup() {
  // Find the address of the I/O link device.
  RdmaDevicePtr linkDevice;
  try {
    LOG_DEBUG_MSG("creating InfiniBand device for " << _device << " using interface " << _interface);
    linkDevice = RdmaDevicePtr(new RdmaDevice(_device, _interface));
  } catch (bgcios::RdmaError& e) {
    LOG_ERROR_MSG("error opening InfiniBand device: " << e.what());
    return e.errcode();
  }
  LOG_DEBUG_MSG(
      "created InfiniBand device for " << linkDevice->getDeviceName() << " using interface "
      << linkDevice->getInterfaceName());

  _localAddr       = linkDevice->getAddress();
  _localAddrString = inet_ntoa(*(struct in_addr*)(&_localAddr));
  LOG_DEBUG_MSG("Device returns IP address " << _localAddrString.c_str());

  // Create listener for RDMA connections.
  try {
    //
    // @TODO replace with hpx::util::batch_environment functions
    //
    // if multiple servers are run, they should use the same port, so
    // only rank 0 should set it and the rest, copy it.
    int rank = 0;
    try {
      LOG_DEBUG_MSG("Looking for SLURM_PROCID");
      const char *val = getenv("SLURM_PROCID");
      if (val) rank = boost::lexical_cast<int>(val);
    }
    catch (...) {
      LOG_ERROR_MSG("Failed to parse SLURM_PROCID");
    }
    //
    if (rank == 0) {
      _rdmaListener = bgcios::RdmaServerPtr(new bgcios::RdmaServer(linkDevice->getAddress(), this->_port));
      LOG_DEBUG_MSG("Writing port number to port-server.cfg");
      std::ofstream tempfile("/gpfs/bbp.cscs.ch/home/biddisco/tmp/port-server.cfg");
      tempfile << _rdmaListener->getLocalPort() << std::endl;
    }
    else {
      sleep(1);
      std::ifstream tempfile("/gpfs/bbp.cscs.ch/home/biddisco/tmp/port-server.cfg");
      tempfile >> this->_port;
      LOG_DEBUG_MSG("SLURM_PROCID is " << rank << " reading port from file " << this->_port);
      _rdmaListener = bgcios::RdmaServerPtr(new bgcios::RdmaServer(linkDevice->getAddress(), this->_port));
    }
    if (_rdmaListener->getLocalPort() != this->_port) {
      this->_port = _rdmaListener->getLocalPort();
      LOG_DEBUG_MSG("RdmaServer port changed to " << std::dec << decnumber(this->_port));
    }
  } catch (bgcios::RdmaError& e) {
    LOG_ERROR_MSG("error creating listening RDMA connection: " << e.what());
    return e.errcode();
  }

  LOG_DEBUG_MSG(
      "created listening RDMA connection on port " << decnumber(this->_port) << " IP address " << _localAddrString.c_str());

  // Create a protection domain object.
  try {
    _protectionDomain = RdmaProtectionDomainPtr(new RdmaProtectionDomain(_rdmaListener->getContext()));
  } catch (bgcios::RdmaError& e) {
    LOG_ERROR_MSG("error allocating protection domain: " << e.what());
    return e.errcode();
  }
  LOG_DEBUG_MSG("created protection domain " << _protectionDomain->getHandle());

  // Create a completion channel object.
  try {
    _completionChannel = RdmaCompletionChannelPtr(new RdmaCompletionChannel(_rdmaListener->getContext(), false));
  } catch (bgcios::RdmaError& e) {
    LOG_ERROR_MSG("error constructing completion channel: " << e.what());
    return e.errcode();
  }

  // Create a memory pool for pinned buffers
  _memoryPool = std::make_shared < RdmaMemoryPool > (_protectionDomain,
          RDMA_DEFAULT_MEMORY_POOL_SMALL_CHUNK_SIZE, RDMA_DEFAULT_CHUNKS_ALLOC, RDMA_MAX_CHUNKS_ALLOC);

#ifdef USE_SHARED_RECEIVE_QUEUE
  // create a shared receive queue
  LOG_DEBUG_MSG("Creating SRQ shared receive queue ");
  _rdmaListener->create_srq(_protectionDomain);
#endif

  // Listen for connections.
  int err = _rdmaListener->listen(256);
  if (err != 0) {
    LOG_ERROR_MSG("error listening for new RDMA connections: " << RdmaError::errorString(err));
    return err;
  }
  LOG_DEBUG_MSG("listening for new RDMA connections on fd " << hexnumber(_rdmaListener->getEventChannelFd()));

  // Create a large memory region.
  _largeRegion = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
  err = _largeRegion->allocate(_protectionDomain, LargeRegionSize);
  if (err != 0) {
    LOG_ERROR_MSG("error allocating large memory region: " << RdmaError::errorString(err));
    return err;
  }
  LOG_DEBUG_MSG("created large memory region with local key " << _largeRegion->getLocalKey());

  return 0;
}
/*---------------------------------------------------------------------------*/
int RdmaController::cleanup(void) {
  return 0;
}

/*---------------------------------------------------------------------------*/
void RdmaController::freeRegion(RdmaMemoryRegion *region) {
  LOG_ERROR_MSG("Removed region free code, must replace it");
  //  if(!this->_memoryPool->is_from(region)) {
  //    throw std::runtime_error("Trying to delete a meory region we didn't allocate");
  //  }
  //  LOG_DEBUG_MSG("")
  //  this->_memoryPool->free(region);
}

/*---------------------------------------------------------------------------*/
void RdmaController::refill_client_receives() {
  // make sure all clients have a pre-posted receive in their queues
  std::for_each(_clients.begin(), _clients.end(), [](RdmaController::ClientMapPair _client) {
    _client.second->refill_preposts(RDMA_MAX_PREPOSTS);
  });
}

/*---------------------------------------------------------------------------*/
int RdmaController::pollCompletionQueues()
{
//    LOG_DEBUG_MSG("pollCompletionQueues");
    //
    int ntot = 0, nc = 0;
    //
    for (auto _client : _clients) {
        // avoid using smartpointers to save atomic ops
        RdmaClient *client = _client.second.get();
        RdmaCompletionQueue *completionQ = client->getCompletionQ().get();

        // Remove work completions from the completion queue until it is empty.
        do {
            struct ibv_wc completion;
            nc = completionQ->poll_completion(&completion);
            if (nc>0) {
                if (completion.status != IBV_WC_SUCCESS) {
                    LOG_ERROR_MSG("Message failed currect receive count is " << client->getNumReceives());
                    LOG_DEBUG_MSG("pollCompletionQueues - removing wr_id " << hexpointer(completion.wr_id) << " "
                            << RdmaCompletionQueue::wc_opcode_str(completion.opcode));
                    std::terminate();
                }
                if (this->_completionFunction) {
                    this->_completionFunction(completion, client);
                }
            }
            ntot += nc;
        } while (nc>0);
    }
    return ntot;
}
/*---------------------------------------------------------------------------*/
int RdmaController::pollEventChannel()
{
    return hpx::parcelset::policies::verbs::event_channel::
        poll_event_channel(_rdmaListener->getEventChannelFd(),
            [this](){
                this->eventChannelHandler();
            }
        );
}
/*---------------------------------------------------------------------------*/
int RdmaController::eventMonitor(int Nevents)
{
  int events_handled = 0;
  events_handled += pollCompletionQueues();
  if (event_poll_count++ % 10 == 0) {
    events_handled += pollEventChannel();
  }
  return events_handled;
}

/*---------------------------------------------------------------------------*/
void RdmaController::eventChannelHandler(void)
{
  // Wait for the event (it should be here now).
  int err = _rdmaListener->waitForEvent();

  if (err != 0) {
    return;
  }

  // Handle the event.
  rdma_cm_event_type type = _rdmaListener->getEventType();

  switch (type) {

  case RDMA_CM_EVENT_CONNECT_REQUEST: {
    LOG_DEBUG_MSG("RDMA_CM_EVENT_CONNECT_REQUEST");
    //
    // prevent
    //
    if (this->_preConnectionFunction) {
        struct sockaddr *ip_src = &_rdmaListener->getEventId()->route.addr.src_addr;
        struct sockaddr_in *addr_src = reinterpret_cast<struct sockaddr_in *>(ip_src);
        //
        struct sockaddr *ip_dst = &_rdmaListener->getEventId()->route.addr.dst_addr;
        struct sockaddr_in *addr_dst = reinterpret_cast<struct sockaddr_in *>(ip_dst);
        //
        std::string ip_dsts = inet_ntoa(addr_dst->sin_addr);
        std::string ip_srcs = inet_ntoa(addr_src->sin_addr);
        //
        LOG_DEBUG_MSG("calling connection callback for IP " << ip_dsts.c_str() << " " << ip_srcs.c_str());
        //
        char ip1[4], ip2[4];
        memcpy(ip1, &addr_dst->sin_addr, 4);
        memcpy(ip2, &addr_src->sin_addr, 4);
        //
        LOG_DEBUG_MSG("Address source " << ipaddress(ip1) << " Address dst " << ipaddress(ip2));
        if (!this->_preConnectionFunction()) {
            LOG_ERROR_MSG("We are already accepting a connection, so reject");
            _rdmaListener->reject(_rdmaListener->getEventId());
        }
    }

    // Construct a RdmaCompletionQueue object for the new client.
    RdmaCompletionQueuePtr completionQ;
    try {

//      completionQ = std::make_shared < RdmaCompletionQueue
//          > (_rdmaListener->getEventContext(), RdmaCompletionQueue::MaxQueueSize, _completionChannel->getChannel());
      completionQ = std::make_shared < RdmaCompletionQueue
          > (_rdmaListener->getEventContext(), RdmaCompletionQueue::MaxQueueSize, (ibv_comp_channel*)NULL);
    } catch (bgcios::RdmaError& e) {
      LOG_ERROR_MSG("error creating completion queue: " << e.what());
      return;
    }

    // Construct a new RdmaClient object for the new client.
    RdmaClientPtr client;
    try {
      client = std::make_shared < RdmaClient
          > (_rdmaListener->getEventId(), _protectionDomain, completionQ, _memoryPool, _rdmaListener->SRQ());
    } catch (bgcios::RdmaError& e) {
      LOG_ERROR_MSG("error creating rdma client: %s\n" << e.what());
      completionQ.reset();
      return;
    }

    LOG_DEBUG_MSG("adding a new client with qpnum " << client->getQpNum());
    // Add new client to map of active clients.
    _clients[client->getQpNum()] = client;

    // Add completion queue to completion channel.
//    _completionChannel->addCompletionQ(completionQ);

    this->refill_client_receives();

    // Accept the connection from the new client.
    err = client->accept();
    if (err != 0) {
      printf("error accepting client connection: %s\n", RdmaError::errorString(err));
      _clients.erase(client->getQpNum());
//      _completionChannel->removeCompletionQ(completionQ);
      client->reject(_rdmaListener->getEventId()); // Tell client the bad news
      client.reset();
      completionQ.reset();
      break;
    }
    LOG_DEBUG_MSG("accepted connection from " << client->getRemoteAddressString().c_str());

    break;
  }

  case RDMA_CM_EVENT_ESTABLISHED: {
    LOG_DEBUG_MSG("RDMA_CM_EVENT_ESTABLISHED");
    // Find connection associated with this event.
    RdmaClientPtr client = _clients[_rdmaListener->getEventQpNum()];
    LOG_CIOS_INFO_MSG(client->getTag() << "connection established with " << client->getRemoteAddressString());
    if (this->_connectionFunction) {
        LOG_DEBUG_MSG("calling connection callback ");
        this->_connectionFunction(std::make_pair(client->getQpNum(), 0), client);
    }
    break;
  }

  case RDMA_CM_EVENT_DISCONNECTED: {
    LOG_DEBUG_MSG("RDMA_CM_EVENT_DISCONNECTED");
    // Find connection associated with this event.
    uint32_t qp = _rdmaListener->getEventQpNum();
    RdmaClientPtr client = _clients[qp];
    RdmaCompletionQueuePtr completionQ = client->getCompletionQ();
    /*
     while (client->getNumReceives()>0 ) {
     //           LOG_ERROR_MSG("@@@ ERROR there are uncompleted events NumWaitingRecv " << client->getNumWaitingRecv() << " NumWaitingSend " << client->getNumWaitingSend());
     //           this->eventMonitor(1,false);
     //           LOG_ERROR_MSG("$$$ ERROR there are uncompleted events NumWaitingRecv " << client->getNumWaitingRecv() << " NumWaitingSend " << client->getNumWaitingSend());
     while (completionQ->removeCompletions() != 0) {
     LOG_ERROR_MSG("Removed a completion from the queue ");
     // Get the next work completion.
     struct ibv_wc *completion = completionQ->popCompletion();
     LOG_DEBUG_MSG("Removing wr_id " << std::setfill('0') << std::setw(12) << std::hex << completion->wr_id);
     // Find the connection that received the message.
     //              client = _clients.get(completion->qp_num);
     }
     //              if (this->_completionFunction) {
     //                this->_completionFunction(completion, client);
     //              }
     }

     // we must not disconnect if there are outstanding work requests
     LOG_ERROR_MSG("@@@ CLEAR uncompleted events handled before disconnection");
     */

    // Complete disconnect initiated by peer.
    err = client->disconnect(false);
    if (err == 0) {
      LOG_CIOS_INFO_MSG(client->getTag() << "disconnected from " << client->getRemoteAddressString());
    } else {
      LOG_ERROR_MSG(client->getTag() << "error disconnecting from peer: " << RdmaError::errorString(err));
    }

    // Acknowledge the event (must be done before removing the rdma cm id).
    _rdmaListener->ackEvent();

    // Remove connection from map of active connections.
    _clients.erase(qp);

    // Destroy connection object.
    LOG_DEBUG_MSG("destroying RDMA connection to client " << client->getRemoteAddressString());
    client.reset();

    // Remove completion queue from the completion channel.
//    _completionChannel->removeCompletionQ(completionQ);

    // Destroy the completion queue.
    LOG_DEBUG_MSG("destroying completion queue " << completionQ->getHandle());
    completionQ.reset();

    break;
  }

  default: {
    LOG_ERROR_MSG("RDMA event: " << rdma_event_str(type) << " is not supported");
    break;
  }
  }

  // Acknowledge the event.  Should this always be done?
  if (type != RDMA_CM_EVENT_DISCONNECTED) {
    _rdmaListener->ackEvent();
  }

  return;
}

/*---------------------------------------------------------------------------*/
bool RdmaController::completionChannelHandler(uint64_t requestId) { //, lock_type2 &&lock) {
    RdmaClient *client;
    try {
        // Get the notification event from the completion channel.
        RdmaCompletionQueue *completionQ = _completionChannel->getEvent();

        // Remove work completions from the completion queue until it is empty.
        while (completionQ->removeCompletions() != 0) {
            // Get the next work completion.
            struct ibv_wc *completion;
            // the completion queue isn't yet thread safe, so only allow one thread at a time to pop a completion
            {
                completion = completionQ->popCompletion();
                LOG_DEBUG_MSG("Controller completion - removing wr_id " << hexpointer(completion->wr_id));
                // Find the connection that received the message.
                client = _clients[completion->qp_num].get();
            }
            if (this->_completionFunction) {
//                this->_completionFunction(completion, client);
            }
        }
    }

    catch (const RdmaError& e) {
        LOG_ERROR_MSG("error removing work completions from completion queue: " << RdmaError::errorString(e.errcode()));
    }

    return true;
}

/*---------------------------------------------------------------------------*/
// return the client
RdmaClientPtr RdmaController::makeServerToServerConnection(uint32_t remote_ip, uint32_t remote_port)
{
  std::string remoteAddr = inet_ntoa(*(struct in_addr*)(&remote_ip));
  std::string remotePort = boost::lexical_cast<std::string>(remote_port);

  RdmaCompletionQueuePtr completionQ;
  try {
//    completionQ = std::make_shared < RdmaCompletionQueue >
//      (_rdmaListener->getContext(), RdmaCompletionQueue::MaxQueueSize, _completionChannel->getChannel());
    completionQ = std::make_shared < RdmaCompletionQueue >
      (_rdmaListener->getContext(), RdmaCompletionQueue::MaxQueueSize, (ibv_comp_channel*)NULL);
  } catch (bgcios::RdmaError& e) {
    LOG_ERROR_MSG("error creating completion queue: " << e.what());
  }

  RdmaClientPtr newClient;
  try {
    newClient = std::make_shared<RdmaClient>
      (_localAddrString, _localPortString, remoteAddr, remotePort);
  } catch (bgcios::RdmaError& e) {
    LOG_ERROR_MSG("error creating rdma client: %s\n" << e.what());
    completionQ.reset();
  }

  // make a connection
  newClient->makePeer(_protectionDomain, completionQ);

  // Add new client to map of active clients.
  _clients[newClient->getQpNum()] = newClient;

  // Add completion queue to completion channel.
//  _completionChannel->addCompletionQ(completionQ);

  newClient->setMemoryPool(_memoryPool);
  newClient->setInitiatedConnection(true);
  this->refill_client_receives();

  LOG_DEBUG_MSG("Added a server-server client with qpnum " << decnumber(newClient->getQpNum()));
  return newClient;
}

/*---------------------------------------------------------------------------*/
void RdmaController::removeServerToServerConnection(RdmaClientPtr client)
{
    LOG_DEBUG_MSG("Removing Server-Server client object");
    // Find connection associated with this event.
    RdmaCompletionQueuePtr completionQ = client->getCompletionQ();
    uint32_t qp = client->getQpNum();

    // disconnect initiated by us
    int err = client->disconnect(true);
    if (err == 0) {
      LOG_CIOS_INFO_MSG(client->getTag() << "disconnected from " << client->getRemoteAddressString());
    } else {
      LOG_ERROR_MSG(client->getTag() << "error disconnecting from peer: " << RdmaError::errorString(err));
    }

    // Remove connection from map of active connections.
    _clients.erase(qp);

    // Destroy connection object.
    LOG_DEBUG_MSG("destroying RDMA connection to client " << client->getRemoteAddressString());
    client.reset();

    // Remove completion queue from the completion channel.
//    _completionChannel->removeCompletionQ(completionQ);

    // Destroy the completion queue.
    LOG_DEBUG_MSG("destroying completion queue " << completionQ->getHandle());
    completionQ.reset();
}

/*---------------------------------------------------------------------------*/
void RdmaController::removeAllInitiatedConnections()
{
    while (std::count_if(_clients.begin(), _clients.end(),
            [](const std::pair<uint32_t, RdmaClientPtr> & c) {
                return c.second->getInitiatedConnection();
            })>0)
    {
        std::map<uint32_t, RdmaClientPtr>::iterator c = _clients.begin();
        while (c != _clients.end()) {
            if (c->second->getInitiatedConnection()) {
                LOG_DEBUG_MSG("Removing a connection");
                removeServerToServerConnection(c->second);
                break;
            }
        }
    }
}
