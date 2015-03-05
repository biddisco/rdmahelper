//
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */

//! \file  RdmaConnectioniBase.h
//! \brief Declaration and inline methods for bgcios::RdmaClient class.
#ifndef COMMON_RDMACONNECTION_BASE_H
#define COMMON_RDMACONNECTION_BASE_H

// Includes
#include <memory>
#include "memory_pool.h"
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <queue>

//! Base connection class for RDMA operations with a remote partner.
namespace bgcios {

class RdmaConnectionBase {
public:
  // expected ops must be stored per client, so use a map with qp as key
  typedef std::queue<struct na_verbs_op_id*> OperationQueue;
  OperationQueue ExpectedOps;
  std::queue<uint64_t> _waitingReceives;

  //
  RdmaConnectionBase() { }

  virtual ~RdmaConnectionBase() {
    while (!_waitingReceives.empty()) {
      LOG_DEBUG_MSG("Connection closing, releasing region");
      RdmaMemoryRegion *region = (RdmaMemoryRegion *)_waitingReceives.front();
      this->releaseRegion(region);
      _waitingReceives.pop();
    }
    // clear memory pool reference
    LOG_DEBUG_MSG("releasing memory pool reference");
    _memoryPool.reset();
  }

  /*---------------------------------------------------------------------------*/
  uint64_t popReceive() {
    LOG_DEBUG_MSG("Before pop size of waiting receives is " << _waitingReceives.size())
    uint64_t wr_id = _waitingReceives.front();
    LOG_DEBUG_MSG("Before pop size of waiting receives is " << _waitingReceives.size())
    this->_waitingReceives.pop();
    LOG_DEBUG_MSG("Before pop size of waiting receives is " << _waitingReceives.size())
    LOG_DEBUG_MSG("After pop of " << hexpointer(wr_id) << " size of waiting receives is " << _waitingReceives.size())
    return wr_id;
  }

  /*---------------------------------------------------------------------------*/
  void pushReceive_(uint64_t wr_id) {
    _waitingReceives.push(wr_id);
    LOG_DEBUG_MSG("After push of " << hexpointer(wr_id) << "size of waiting receives is " << _waitingReceives.size())
  }

  /*---------------------------------------------------------------------------*/
  //! The number of outstanding work requests
  uint32_t getNumReceives() { return _waitingReceives.size(); }

  /*---------------------------------------------------------------------------*/
  void refill_preposts(int preposts) {
    LOG_DEBUG_MSG("Entering refill size of waiting receives is " << _waitingReceives.size())
    while (this->getNumReceives()<preposts) {
      LOG_DEBUG_MSG("Pre-Posting a receive to client");
      RdmaMemoryRegion *region = this->getFreeRegion(512);
      this->postRecvRegionAsID(region, region->getLength(), false);
    }
  }

  /*---------------------------------------------------------------------------*/
  void setMemoryPool(memory_poolPtr pool)
  {
    this->_memoryPool = pool;
  }

  /*---------------------------------------------------------------------------*/
  RdmaMemoryRegion *getFreeRegion(size_t size)
  {
    RdmaMemoryRegion* region = this->_memoryPool->allocate(size).get();
    if (!region) {
      LOG_ERROR_MSG("Error creating free memory region");
    }
    region->setMessageLength(size);

    return region;
  }

  /*---------------------------------------------------------------------------*/
  void releaseRegion(RdmaMemoryRegion *region)
  {
   this->_memoryPool->deallocate(region);
  }

  /*---------------------------------------------------------------------------*/
  virtual uint64_t
  postRecvRegionAsID(RdmaMemoryRegion *region, uint32_t length, bool expected=false) = 0;

protected:
  memory_poolPtr _memoryPool;

};

} // namespace bgcios

#endif // COMMON_RDMACONNECTION_BASE_H
