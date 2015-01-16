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

//! \file  MercuryController.h
//! \brief Declaration for bgcios::stdio::MercuryController class.

#ifndef STDIO_HWSTDIOCONTROLLER_H
#define STDIO_HWSTDIOCONTROLLER_H

#include "rdmahelper_defines.h"
#include "rdmahelper_logging.h"
//
#include <ramdisk/include/services/common/PointerMap.h>
#include <ramdisk/include/services/common/RdmaCompletionChannel.h>
#include <ramdisk/include/services/common/RdmaClient.h>
#include <ramdisk/include/services/common/RdmaServer.h>
#include <ramdisk/include/services/common/RdmaError.h>
#include <memory>
#include <deque>
#include <chrono>
#include <iostream>
#include <functional>

namespace bgcios
{

namespace stdio
{

//! \brief Handle standard I/O service messages when running on hardware.

class MercuryController
{
public:
  typedef std::pair<uint32_t, RdmaClientPtr> ClientMapPair;

   //! \brief  Default constructor.
   //! \param  config Configuration from command line and properties file.

   MercuryController(const char *device, const char *interface, int port);

   //! \brief  Default destructor.

   ~MercuryController();

   //! \brief  Open all connections needed by the service daemon.
   //! \param  dataChannelPort Port number for listening data channel socket.
   //! \return 0 when successful, errno when unsuccessful.

   int startup();

   bool isTerminated() { return (_clients.size()==0); }

   typedef std::function<int(std::pair<uint32_t,uint64_t>, RdmaClientPtr client)> ConnectionFunction;
   void setConnectionFunction(ConnectionFunction f) { this->_connectionFunction = f;}

   //! \brief  Close all connections needed by the service daemon.
   //! \return 0 when successful, errno when unsuccessful.

   int cleanup(void);

   //! \brief  Monitor for events from all channels.
   //! \return Nothing.

   void eventMonitor(int Nevents);

   //! Listener for RDMA connections.
   bgcios::RdmaServerPtr getServer() { return this->_rdmaListener; }
   bgcios::RdmaProtectionDomainPtr getProtectionDomain() { return this->_protectionDomain; }
   bgcios::RdmaClientPtr getClient(uint32_t qp) {
     return _clients.get(qp);
   }

   template <typename Function>
   void for_each_client(Function lambda)
   {
     std::for_each(_clients.begin(), _clients.end(), lambda);
   }

   bgcios::RdmaCompletionChannelPtr GetCompletionChannel() { return this->_completionChannel; }

   int getPort() { return _port; }

   typedef std::function<int(struct ibv_wc *completion, RdmaClientPtr client)> CompletionFunction;
   void setCompletionFunction(CompletionFunction f) { this->_completionFunction = f;}

   void freeRegion(RdmaMemoryRegion *region);

private:

   void eventChannelHandler(void);

   //! \brief  Handle events from completion channel.
   //! \return Nothing.

   bool completionChannelHandler(uint64_t requestId);

   std::string _device;
   std::string _interface;
   int         _port;

   CompletionFunction _completionFunction;
   ConnectionFunction _connectionFunction;

   //! Listener for RDMA connections.
   bgcios::RdmaServerPtr _rdmaListener;

   //! Protection domain for all resources.
   bgcios::RdmaProtectionDomainPtr _protectionDomain;

   //! Completion channel for all completion queues.
   bgcios::RdmaCompletionChannelPtr _completionChannel;

   //! Map of all active clients indexed by queue pair number.
   bgcios::PointerMap<uint32_t, RdmaClientPtr> _clients;

   //! Large memory region for transferring data (used for both inbound and outbound data).
   bgcios::RdmaMemoryRegionPtr _largeRegion;

   memory_poolPtr _memoryPool;

   //! \brief  Transfer data to the client from the large memory region.
   //! \param  address Address of remote memory region.
   //! \param  rkey Key of remote memory region.
   //! \param  length Length of data to transfer.
   //! \return 0 when successful, error when unsuccessful.
/*
   uint32_t putData(const RdmaClientPtr& client, uint64_t address, uint32_t rkey, uint32_t length)
   {
       uint32_t rc = 0;
   try {
      // Post a rdma write request to the send queue using the large message region.
           _largeRegion->setMessageLength(length);
           uint64_t reqID = (uint64_t)_largeRegion->getAddress();
           uint64_t& localAddress = reqID;
           uint32_t lkey = _largeRegion->getLocalKey();
           int err = client->postRdmaWrite(reqID, rkey, address, //remote key and address
                     lkey,  localAddress, (ssize_t)length,IBV_SEND_SIGNALED);
           if (err) return (rc=(uint32_t)err);

           // Wait for notification that the rdma read completed.
           while (!completionChannelHandler(reqID));
       }

       catch (const RdmaError& e) {
           rc = (uint32_t)e.errcode();
       }

       return rc;
   }

   //! \brief  Transfer data from the client into the large memory region.
   //! \param  address Address of remote memory region.
   //! \param  rkey Key of remote memory region.
   //! \param  length Length of data to transfer.
   //! \return 0 when successful, error when unsuccessful.

   uint32_t getData( const RdmaClientPtr& client, uint64_t address, uint32_t rkey, uint32_t length)
   {
       uint32_t rc = 0;
       try {
           // Post a rdma read request to the send queue using the large message region.
           std::cout << "Here 1 (getData) " << std::endl;
           _largeRegion->setMessageLength(length);
           std::cout << "Here 2 " << std::endl;
           uint64_t reqID = (uint64_t)_largeRegion->getAddress();
           std::cout << "Here 3 " << std::endl;
           uint64_t& localAddress = reqID;
           std::cout << "Here 4 " << std::endl;
           uint32_t lkey = _largeRegion->getLocalKey();
           std::cout << "Here 5 " << std::endl;
           int err = client->postRdmaRead(reqID, rkey, address, //remote key and address
                                           lkey,  localAddress, (ssize_t)length);
           std::cout << "Here 6 " << std::endl;
           if (err) return (rc=(uint32_t)err);
           std::cout << "Here 7 " << std::endl;

           // Wait for notification that the rdma read completed.
           while (!completionChannelHandler(reqID));
       }

       catch (const RdmaError& e) {
           rc = (uint32_t)e.errcode();
           std::cout << "Caught an exception 8 " << std::endl;
       }

       return rc;
   }
*/

};

//! Smart pointer for MercuryController object.
typedef std::shared_ptr<MercuryController> MercuryControllerPtr;

} // namespace stdio

} // namespace bgcios

#endif // STDIO_STDIOCONTROLLER_H

