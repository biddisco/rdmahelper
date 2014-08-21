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

//! \file  RdmaServer.cc 
//! \brief Methods for bgcios::RdmaServer class.

// Includes
#include <ramdisk/include/services/common/RdmaServer.h>
#include <ramdisk/include/services/common/RdmaError.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <ramdisk/include/services/common/logging.h>

using namespace bgcios;

LOG_DECLARE_FILE("cios.common");

RdmaServer::RdmaServer(in_addr_t addr, in_port_t port) : RdmaConnection()
{
   // Generate an unique tag for trace points.
   std::ostringstream tag;
   tag << "[CM " << port << "] ";
   setTag(tag.str());

   // Create the RDMA connection management id and event channel.
   LOG_CIOS_DEBUG_MSG(_tag << "ready to create rdma cm id");
   createId();

   // Bind an address for connections.
   int err = bind(addr, port);
   if (err != 0) {
      RdmaError e(err, "bind() failed");
      LOG_ERROR_MSG(_tag << "error binding to port " << port << ": " << bgcios::errorString(e.errcode()));
      throw e;
   }

}

int
RdmaServer::bind(in_addr_t addr, in_port_t port)
{
   // Bind an address to the listening connection.
   memset(&_localAddress, 0, sizeof(_localAddress));
   _localAddress.sin_family = PF_INET;
   _localAddress.sin_addr.s_addr = addr;
   _localAddress.sin_port = htons(port);
   int err = rdma_bind_addr(_cmId, (struct sockaddr *)&_localAddress);
   if (err != 0) {
      err = abs(err);
      LOG_ERROR_MSG(_tag << "error binding to address " << addressToString(&_localAddress) << ": " << bgcios::errorString(err));
      abort();
      return err;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "bound rdma cm id to address " << addressToString(&_localAddress));
   return 0;
}

int
RdmaServer::listen(int backlog)
{
   // Start listening for connections.
   int err = rdma_listen(_cmId, backlog);
   if (err != 0) {
      err = abs(err);
      LOG_ERROR_MSG(_tag << "error listening for connections: " << bgcios::errorString(err));
      return err;
   }

   LOG_CIOS_DEBUG_MSG(_tag << "listening for connections with backlog " << backlog);
   return 0;
}

