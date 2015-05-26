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
#include "RdmaMemoryPool.h"
#include <iomanip>
#include <atomic>

//! Base connection class for RDMA operations with a remote partner.
namespace bgcios {

class RdmaConnectionBase {
public:
    //
    RdmaConnectionBase() : _waitingReceives(0) { }

    virtual ~RdmaConnectionBase() {
        if (_waitingReceives>0) {
            LOG_ERROR_MSG("Closing connection whilst receives are still pending : implement FLUSH");
        }
        LOG_DEBUG_MSG("releasing memory pool reference");
        _memoryPool.reset();
    }

    /*---------------------------------------------------------------------------*/
    void popReceive() const {
        uint64_t temp = --_waitingReceives;
        LOG_DEBUG_MSG("After decrement size of waiting receives is " << decnumber(temp));
    }

    /*---------------------------------------------------------------------------*/
    void pushReceive() const {
        uint64_t temp = ++_waitingReceives;
        LOG_DEBUG_MSG("After increment size of waiting receives is " << decnumber(temp));
    }

    /*---------------------------------------------------------------------------*/
    //! The number of outstanding work requests
    uint32_t getNumReceives() { return _waitingReceives; }

    /*---------------------------------------------------------------------------*/
    void refill_preposts(unsigned int preposts) {
        LOG_DEBUG_MSG("Entering refill size of waiting receives is " << decnumber(_waitingReceives));
        while (this->getNumReceives()<preposts) {
            LOG_DEBUG_MSG("Pre-Posting a receive to client");
            RdmaMemoryRegion *region = this->getFreeRegion(this->_memoryPool->default_chunk_size());
            this->postRecvRegionAsID(region, region->getLength(), false);
        }
    }

    /*---------------------------------------------------------------------------*/
    void setMemoryPool(RdmaMemoryPoolPtr pool)
    {
        this->_memoryPool = pool;
    }

    /*---------------------------------------------------------------------------*/
    RdmaMemoryRegion *getFreeRegion(size_t size)
    {
        RdmaMemoryRegion* region = this->_memoryPool->allocateRegion(size);
        if (!region) {
            LOG_ERROR_MSG("Error creating free memory region");
        }
        region->setMessageLength(size);

        return region;
    }

    /*---------------------------------------------------------------------------*/
    virtual uint64_t
    postRecvRegionAsID(RdmaMemoryRegion *region, uint32_t length, bool expected=false) = 0;

protected:
    RdmaMemoryPoolPtr             _memoryPool;
    mutable std::atomic<uint64_t> _waitingReceives;

};

} // namespace bgcios

#endif // COMMON_RDMACONNECTION_BASE_H
