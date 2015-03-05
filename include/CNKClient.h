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

//! \file  RdmaClient.h
//! \brief Declaration and inline methods for bgcios::RdmaClient class.
#ifndef COMMON_RDMACLIENT_H
#define COMMON_RDMACLIENT_H

// Includes
#include <memory>
#include "RdmaConnectionBase.h"
#include "CNKMemoryRegion.h"
#include "memory_pool.h"
#include <boost/lexical_cast.hpp>
#include <iomanip>

#ifdef __BGQ__
#define ibv_wc_opcode int
#endif

//! Client for RDMA operations with a remote partner.
namespace bgcios {

class RdmaClient : public RdmaConnectionBase {
public:

  //! \brief  Constructor.
  //! \param  localAddr Local address in dotted decimal string format.
  //! \param  localPort Local port number string.
  //! \param  remoteAddr Remote address in dotted decimal string format.
  //! \param  remotePort Remote port number string.
  //! \throws RdmaError.

  RdmaClient(const std::string localAddr, const std::string localPort,
      const std::string remoteAddr, const std::string remotePort);

  //! \brief  Default destructor.

  ~RdmaClient();

  int getFD() {
    return Rdma_FD;
  }

  uint64_t postSend(RdmaMemoryRegion *region, bool signaled, bool withImmediate, uint32_t immediateData)
  {
    void *_address = region->getAddress();

    LOG_DEBUG_MSG(
        "CNK: Posting send with buffer " << hexpointer(_address)
            << " with Length " << hexlength(region->getMessageLength()) );

    int success = Kernel_RDMASend(this->Rdma_FD, _address, region->getMessageLength(), region->getLocalKey());

    if (success != 0) {
      throw("Kernel_RDMASend failed");
    }
    LOG_DEBUG_MSG(
        "posting Send wr_id " << hexpointer(region)
            << " with Length " << hexlength(region->getMessageLength()) << " address "
            << hexpointer(_address));
    return (uint64_t)region;
  }

  uint64_t postRecvRegionAsID(RdmaMemoryRegion *region, uint32_t length, bool expected = false)
  {
    void *_address = region->getAddress();

    int success = Kernel_RDMARecv(this->Rdma_FD, _address, length, region->getLocalKey());
    if (success != 0) {
      throw("Kernel_RDMARecv failed");
    }
    uint64_t wr_id = (uint64_t)region;
    LOG_DEBUG_MSG(
        "posting Recv wr_id " << hexpointer(wr_id)
            << " with Length " << hexlength(length) << " " << hexpointer(_address));
    this->pushReceive_(wr_id);
    return wr_id;
  }

  //! \brief  Connect to a remote server.
  //! \param  domain Protection domain for new connection.
  //! \param  completionQ Completion queue for both send and receive operations.
  //! \return 0 when successful, errno when unsuccessful.

  int makePeer();
  int disconnect(bool initiate) {
    return 0;
  }

  void createRegions();

private:

  //! Memory region for inbound messages.
  RdmaMemoryRegionPtr _inMessageRegion;

  //! Memory region for outbound messages.
  RdmaMemoryRegionPtr _outMessageRegion;

  //! Memory region for Auxilliary outbound messages.
  RdmaMemoryRegionPtr _outMessageRegionAux;

  int Rdma_FD;
  int port;
  //std::shared_ptr<pinned_pool> _pinned_memory_pool;
};

//! Smart pointer for RdmaClient object.
typedef std::shared_ptr<RdmaClient> RdmaClientPtr;

}
;

#endif // COMMON_RDMACLIENT_H

