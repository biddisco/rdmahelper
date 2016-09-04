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

//! \file  RdmaServer.h
//! \brief Declaration for bgcios::RdmaServer class.

#ifndef COMMON_RDMASERVER_H
#define COMMON_RDMASERVER_H

#include <plugins/parcelport/verbs/rdmahelper/include/RdmaConnection.h>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaSharedReceiveQueue.h>
#include <netinet/in.h>
#include <memory>

namespace bgcios
{

//! \brief RDMA server for accepting RDMA connections.

class RdmaServer : public RdmaConnection
{

public:

   //! \brief  Default constructor.

   RdmaServer() : RdmaConnection() { }

   //! \brief  Constructor.
   //! \param  addr IPv4 address to bind to.
   //! \param  port Port number.
   //! \throws rdma_error.

   RdmaServer(in_addr_t addr, in_port_t port);

   //! \brief  Default destructor.

   ~RdmaServer() { }

   //! \brief  Bind an address to listen for connections.
   //! \param  addr IPv4 address to bind to.
   //! \param  port Port number.
   //! \return 0 when successful, errno when unsuccessful.

   int bind(in_addr_t addr, in_port_t port);

   //! \brief  Enable listening for connections from clients.
   //! \param  backlog Number incoming connections to put in backlog.
   //! \return 0 when successful, errno when unsuccessful.

   int listen(int backlog);

   // overridden to support shared receive queue
   virtual uint64_t
   postRecvRegionAsID(rdma_memory_region *region, uint32_t length, bool expected=false)
   {
     struct ibv_srq *srq = _srq->get_SRQ();
     if (!srq) {
       return RdmaConnection::postRecvRegionAsID(region, length, expected);
     }

     // Build scatter/gather element for inbound message.
     struct ibv_sge recv_sge;
     recv_sge.addr   = (uint64_t)region->getAddress();
     recv_sge.length = length;
     recv_sge.lkey   = region->getLocalKey();

     // Build receive work request.
     struct ibv_recv_wr recv_wr;
     memset(&recv_wr, 0, sizeof(recv_wr));
     recv_wr.next    = NULL;
     recv_wr.sg_list = &recv_sge;
     recv_wr.num_sge = 1;
     recv_wr.wr_id   = (uint64_t)region;
     ++_totalRecvPosted;
     struct ibv_recv_wr *badRequest;
     int err = ibv_post_srq_recv(srq, &recv_wr, &badRequest);
     if (err!=0) {
       throw(rdma_error(err, "postSendNoImmed SRQ failed"));
     }
     LOG_DEBUG_MSG(_tag.c_str() << "posting SRQ Recv wr_id " << hexpointer(recv_wr.wr_id) << " with Length " << hexlength(length) << " " << hexpointer(region->getAddress()));
     return recv_wr.wr_id;
   }
};

//! Smart pointer for RdmaServer object.
typedef std::shared_ptr<RdmaServer> RdmaServerPtr;

} // namespace bgcios

#endif // COMMON_RDMASERVER_H
