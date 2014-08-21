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

#include "RdmaConnection.h"
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
   //! \throws RdmaError.

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

};

//! Smart pointer for RdmaServer object.
typedef std::shared_ptr<RdmaServer> RdmaServerPtr;

} // namespace bgcios

#endif // COMMON_RDMASERVER_H
