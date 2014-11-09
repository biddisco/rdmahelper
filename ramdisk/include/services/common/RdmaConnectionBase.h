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
#include "ramdisk/include/services/common/memory_pool.hpp"
#include "ramdisk/include/services/ServicesConstants.h"
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <mutex>
#include <atomic>

//! Base connection class for RDMA operations with a remote partner.
namespace bgcios {

class RdmaConnectionBase {
public:
  //
  RdmaConnectionBase() :
    _waitingExpectedRecvPosted(0),
    _waitingUnexpectedRecvPosted(0),
    _waitingSendPosted(0) {}
  //
  std::deque<void*> _unexpected_recv;

  //! The number of outstanding work requests
  std::atomic<uint32_t> _waitingExpectedRecvPosted;
  std::atomic<uint32_t> _waitingUnexpectedRecvPosted;
  std::atomic<uint32_t> _waitingSendPosted;

   //! \brief  Decrease waiting receive counter
   //! \return the value of the waiting receive counter after decrement
  uint32_t decrementWaitingRecv(bool expected) {
    return (expected) ? (--_waitingExpectedRecvPosted) : (--_waitingUnexpectedRecvPosted);
  }
  uint32_t decrementWaitingSend() { return --_waitingSendPosted; }

   //! \brief  Get the waiting receive counter
   //! \return the value of the waiting receive counter after decrement
   uint32_t getNumWaitingExpectedRecv() { return _waitingExpectedRecvPosted; }
   uint32_t getNumWaitingUnexpectedRecv() { return _waitingUnexpectedRecvPosted; }
   uint32_t getNumWaitingSend() { return _waitingSendPosted; }

   //
  void push_unexpected(void *item) {
    std::lock_guard<std::mutex> lock(_unexpected_lock);
    _unexpected_recv.push_back(item);
  }
  //
  void *pop_unexpected() {
    void *res = NULL;
    std::lock_guard<std::mutex> lock(_unexpected_lock);
    if (_unexpected_recv.size()>0) {
      res = _unexpected_recv.front();
      _unexpected_recv.pop_front();
    }
    return res;
  }
  //
  int unexpected_size() { return _unexpected_recv.size(); }
  std::mutex _unexpected_lock;
};

} // namespace bgcios

#endif // COMMON_RDMACONNECTION_BASE_H
