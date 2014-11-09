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
#include "ramdisk/include/services/common/RdmaConnectionBase.h"
#include "ramdisk/include/services/common/CNKMemoryRegion.h"
#include "ramdisk/include/services/common/memory_pool.hpp"
#include "ramdisk/include/services/ServicesConstants.h"
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

  uint64_t postSend(RdmaMemoryRegionPtr region, bool signaled, bool withImmediate, uint32_t immediateData)
  {
    int success = Kernel_RDMASend(this->Rdma_FD, region->getAddress(),
        region->getMessageLength(), region->getLocalKey());
    if (success != 0) {
      throw("Kernel_RDMASend failed");
    }
    LOG_DEBUG_MSG(
        "posting Send wr_id " << std::hex << (uintptr_t) region.get()
            << " with Length " << region->getMessageLength() << " "
            << std::setw(8) << std::setfill('0') << std::hex
            << region->getAddress());
    return (uintptr_t) region.get();
  }

  uint64_t postRecvRegionAsID(RdmaMemoryRegionPtr region, uint64_t address, uint32_t length, bool expected = false)
  {
    int success = Kernel_RDMARecv(this->Rdma_FD, region->getAddress(), length,
        region->getLocalKey());
//   ++_totalRecvPosted;
    if (success != 0) {
      throw("Kernel_RDMARecv failed");
    }
    if (expected) {
     _waitingExpectedRecvPosted++;
    } else {
     _waitingUnexpectedRecvPosted++;
    }
    LOG_DEBUG_MSG(
        "posting Recv wr_id " << std::hex << (uintptr_t) region.get()
            << " with Length " << length << " " << std::setw(8)
            << std::setfill('0') << std::hex << address);
    return (uintptr_t) region.get();
  }

  //! \brief  Connect to a remote server.
  //! \param  domain Protection domain for new connection.
  //! \param  completionQ Completion queue for both send and receive operations.
  //! \return 0 when successful, errno when unsuccessful.

  int makePeer();
  int disconnect(bool initiate) {
    return 0;
  }

  void setMemoryPoold(memory_poolPtr pool) {
    this->_memoryPool = pool;
  }

  void createRegions();

  //! JB. Gets a memory region from the pinned memory pool
  //! throws runtime_error if no free blocks are available.
  RdmaMemoryRegionPtr getFreeRegion(size_t size = 0);

  //! JB. release a region of memory back to the pool
  void releaseRegion(RdmaMemoryRegion *region);

private:

  //! Memory region for inbound messages.
  RdmaMemoryRegionPtr _inMessageRegion;

  //! Memory region for outbound messages.
  RdmaMemoryRegionPtr _outMessageRegion;

  //! Memory region for Auxilliary outbound messages.
  RdmaMemoryRegionPtr _outMessageRegionAux;

  memory_poolPtr _memoryPool;

  int Rdma_FD;
  int port;
  //std::shared_ptr<pinned_pool> _pinned_memory_pool;
};

//! Smart pointer for RdmaClient object.
typedef std::shared_ptr<RdmaClient> RdmaClientPtr;

}
;

#endif // COMMON_RDMACLIENT_H

