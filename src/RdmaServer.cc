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
#include "RdmaLogging.h"
#include <RdmaServer.h>
#include <RdmaError.h>

using namespace bgcios;

//LOG_DECLARE_FILE("cios.common");

RdmaServer::RdmaServer(in_addr_t addr, in_port_t port) : RdmaConnection()
{
   // Generate an unique tag for trace points.
   std::ostringstream tag;
   tag << "[CM " << port << "] ";
   setTag(tag.str());

   // Bind an address for connections.
   int err = bind(addr, port);
   if (err != 0) {
      RdmaError e(err, "bind() failed");
      LOG_ERROR_MSG(_tag << "error binding RdmaServer to port " << port << ": " << RdmaError::errorString(e.errcode()));
      throw e;
   }

}

//#define PORT_REMAP

int
RdmaServer::bind(in_addr_t addr, in_port_t port)
{
   // Bind an address to the listening connection.
   memset(&_localAddress, 0, sizeof(_localAddress));
   _localAddress.sin_family = PF_INET;
   _localAddress.sin_addr.s_addr = addr;
#ifdef PORT_REMAP
   _localAddress.sin_port = htons(port);
   LOG_CIOS_DEBUG_MSG(_tag << "port remapping using " << _localAddress.sin_port << " instead of " << port);
#else
   _localAddress.sin_port = port;
#endif
   LOG_DEBUG_MSG(_tag << "binding to port " << _localAddress.sin_port );
   int err = rdma_bind_addr(_cmId, (struct sockaddr *)&_localAddress);
   if (err != 0) {
      err = abs(err);
      LOG_ERROR_MSG(_tag << "error binding to address " << addressToString(&_localAddress) << ": " << RdmaError::errorString(err));
      abort();
      return err;
   }
   if (port==0) {
     uint16_t port = ntohs(rdma_get_src_port(_cmId));
     LOG_CIOS_DEBUG_MSG(_tag << "Actual port selected by rdmacm is " << std::dec << port);
     // _localAddress.sin_port = port;
     // Generate an unique tag for trace points.
     std::ostringstream tag;
     tag << "[CM " << port << "] ";
     this->setTag(tag.str());
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
      LOG_ERROR_MSG(_tag << "error listening for connections: " << RdmaError::errorString(err));
      return err;
   }
   LOG_CIOS_DEBUG_MSG(_tag << "listening for connections with backlog " << backlog);
   return 0;
}

