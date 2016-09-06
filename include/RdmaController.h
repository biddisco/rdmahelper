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

//! \file  RdmaController.h
//! \brief Declaration for bgcios::stdio::RdmaController class.

#ifndef STDIO_HWSTDIOCONTROLLER_H
#define STDIO_HWSTDIOCONTROLLER_H

//#define USE_SHARED_RECEIVE_QUEUE
#include <hpx/lcos/local/shared_mutex.hpp>
//
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_error.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaCompletionChannel.h>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaClient.h>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaServer.h>
//
#include <plugins/parcelport/verbs/unordered_map.hpp>
//
#include <memory>
#include <deque>
#include <chrono>
#include <iostream>
#include <functional>
#include <map>
#include <atomic>
//
using namespace hpx::parcelset::policies::verbs;
//
namespace bgcios
{

//! \brief Handle standard I/O service messages when running on hardware.

class RdmaController
{
public:
    typedef std::pair<uint32_t, RdmaClientPtr> ClientMapPair;

#ifdef RDMA_HANDLER_HAVE_HPX
    typedef hpx::lcos::local::spinlock               mutex_type;
    typedef std::unique_lock<mutex_type>             unique_lock;
    typedef hpx::lcos::local::condition_variable_any condition_type;
#else
    typedef std::mutex                    mutex_type;
    typedef std::lock_guard<std::mutex>   scoped_lock;
    typedef std::unique_lock<std::mutex>  unique_lock;
#endif

    //! \brief  Default constructor.
    //! \param  config Configuration from command line and properties file.

    RdmaController(const char *device, const char *interface, int port);

    //! \brief  Default destructor.

    ~RdmaController();

    //! \brief  Open all connections needed by the service daemon.
    //! \param  dataChannelPort Port number for listening data channel socket.
    //! \return 0 when successful, errno when unsuccessful.

    int startup();

    bool isTerminated() { return (_clients.size()==0); }

    typedef std::function<int(struct sockaddr_in *, struct sockaddr_in *)>
        ConnectRequestFunction;
    typedef std::function<int(std::pair<uint32_t,uint64_t>, RdmaClientPtr)>
        ConnectionFunction;
    typedef std::function<int(RdmaClientPtr client)>
        DisconnectionFunction;

    // Set a callback which will be called immediately after
    // RDMA_CM_EVENT_CONNECT_REQUEST has been received. This can be useful to
    // prevent two connections being established at the same time.
    void setConnectRequestFunction(ConnectRequestFunction f) { this->_connectRequestFunction = f;}

    // Set a callback which will be called immediately after
    // RDMA_CM_EVENT_ESTABLISHED has been received.
    // This should be used to initialize all structures for handling a new connection
    void setConnectionFunction(ConnectionFunction f) { this->_connectionFunction = f;}

    // currently not used.
    void setDisconnectionFunction(DisconnectionFunction f) { this->_disconnectionFunction = f;}

    //! \brief  Close all connections needed by the service daemon.
    //! \return 0 when successful, errno when unsuccessful.

    int cleanup(void);

    //! \brief  Monitor for events from all channels.
    //! \return Number of events handled

    int eventMonitor(int Nevents);
    int pollCompletionQueues();

    //! Listener for RDMA connections.
    bgcios::RdmaServerPtr getServer() { return this->_rdmaListener; }
    bgcios::rdma_protection_domainPtr getProtectionDomain() { return this->_protectionDomain; }
    bgcios::RdmaClientPtr getClient(uint32_t qp) {
        return _clients[qp];
    }

    rdma_memory_poolPtr getMemoryPool() { return _memoryPool; }

    template <typename Function>
    void for_each_client(Function lambda)
    {
        std::for_each(_clients.begin(), _clients.end(), lambda);
    }

    bgcios::RdmaCompletionChannelPtr GetCompletionChannel() { return this->_completionChannel; }

    unsigned int getPort() { return _port; }

    typedef std::function<int(struct ibv_wc completion, RdmaClient *client)> CompletionFunction;
    void setCompletionFunction(CompletionFunction f) { this->_completionFunction = f;}

    int num_clients() { return _clients.size(); }

    void refill_client_receives();

    RdmaClientPtr makeServerToServerConnection(uint32_t remote_ip, uint32_t remote_port,
        struct ibv_context *cxt);

    void removeServerToServerConnection(RdmaClientPtr client);

    void removeAllInitiatedConnections();
private:

    void eventChannelHandler(void);

    //! \brief  Handle events from completion channel.
    //! \return Nothing.

    bool completionChannelHandler(uint64_t requestId);

    std::string _device;
    std::string _interface;
    int         _port;
    uint32_t    _localAddr; // equivalent to in_addr_t
    std::string _localAddrString;
    std::string _localPortString;
    std::atomic<uint32_t> event_poll_count;

    CompletionFunction       _completionFunction;
    ConnectRequestFunction   _connectRequestFunction;
    ConnectionFunction       _connectionFunction;
    DisconnectionFunction    _disconnectionFunction;

    //! Listener for RDMA connections.
    bgcios::RdmaServerPtr _rdmaListener;

    //! Protection domain for all resources.
    bgcios::rdma_protection_domainPtr _protectionDomain;

    //! Completion channel for all completion queues.
    bgcios::RdmaCompletionChannelPtr _completionChannel;

    //! Map of all active clients indexed by queue pair number.
    hpx::concurrent::unordered_map<uint32_t, RdmaClientPtr> _clients;
    hpx::concurrent::unordered_map<uint32_t, RdmaClientPtr> connecting_clients_;
    typedef hpx::concurrent::unordered_map<uint32_t, RdmaClientPtr>::map_read_lock_type
        map_read_lock_type;

    // only allow one thread to handle connect/disconnect events etc
    mutex_type event_channel_mutex_;

    rdma_memory_poolPtr _memoryPool;

};

//! Smart pointer for RdmaController object.
typedef std::shared_ptr<RdmaController> RdmaControllerPtr;

} // namespace bgcios

#endif // STDIO_STDIOCONTROLLER_H

