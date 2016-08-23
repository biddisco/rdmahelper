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
#include "RdmaLogging.h"
#include "RdmaMemoryPool.h"
//
#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <functional>
//
//----------------------------------------------------------------------------
RdmaMemoryPool::RdmaMemoryPool(protection_domain_type pd, std::size_t chunk_size,
        std::size_t init_chunks, std::size_t max_chunks)
: protection_domain_(pd)
, isServer(true)
, small_ (RDMA_DEFAULT_MEMORY_POOL_SMALL_CHUNK_SIZE,  RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS)
, medium_(RDMA_DEFAULT_MEMORY_POOL_MEDIUM_CHUNK_SIZE, RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS)
, large_ (RDMA_DEFAULT_MEMORY_POOL_LARGE_CHUNK_SIZE,  RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS)
, temp_regions(0)
{
    LOG_DEBUG_MSG("small pool " << small_.chunk_size_ << " or " << small_.chunk_size_);
    auto alloc =
            std::bind(&RdmaMemoryPool::AllocateRegisteredBlock, this, std::placeholders::_1);
    small_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS, alloc);
    medium_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS, alloc);
    large_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS, alloc);
}
//----------------------------------------------------------------------------
RdmaMemoryPool::~RdmaMemoryPool()
{
    DeallocateList();
}
//----------------------------------------------------------------------------
void RdmaMemoryPool::setProtectionDomain(protection_domain_type pd)
{
    DeallocateList();
    protection_domain_ = pd;
    pool_container::regionAllocFunction alloc =
            std::bind(&RdmaMemoryPool::AllocateRegisteredBlock, this, std::placeholders::_1);
    small_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS, alloc);
    medium_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS, alloc);
    large_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS, alloc);
}
//----------------------------------------------------------------------------
char *RdmaMemoryPool::allocate(size_t length)
{
    if (length>large_.chunk_size_) {
        LOG_ERROR_MSG("Chunk pool size exceeded " << length);
        std::terminate();
        throw pinned_memory_exception(std::string(std::string("Chunk pool size exceeded ") + std::to_string(length)).c_str());
    }
    RdmaMemoryRegion *region = allocateRegion(length);
    return static_cast<char*>(region->getAddress());
}

//----------------------------------------------------------------------------
bool RdmaMemoryPool::canAllocateRegionUnsafe(size_t length)
{
    if (length<=small_.chunk_size_) {
        return !small_.free_list_.empty();
    }
    else if (length<=medium_.chunk_size_) {
        return !medium_.free_list_.empty();
    }
    else if (length<=large_.chunk_size_) {
        return !large_.free_list_.empty();
    }
    return true;
}

//----------------------------------------------------------------------------
RdmaMemoryRegion *RdmaMemoryPool::allocateRegion(size_t length)
{
    RdmaMemoryRegion *buffer;
    //
    if (length<=small_.chunk_size_) {
        buffer = small_.pop();
    }
    else if (length<=medium_.chunk_size_) {
        buffer = medium_.pop();
    }
    else if (length<=large_.chunk_size_) {
        buffer = large_.pop();
    }
    else {
        buffer = AllocateTemporaryBlock(length);
//        LOG_ERROR_MSG("Have not yet implemented monster chunk access " << hexnumber(length));
//        std::terminate();
    }

    // initialize with some values for rare debugging issues
#ifdef DEBUG_CHUNK_MEMORY_INIT
    char *addr = static_cast<char *>(buffer->getAddress());
    for (unsigned int i=0; i<length; ++i) {
        addr[i] = (char)(i);
    }
#endif

    LOG_TRACE_MSG("Popping Block"
            << " buffer "    << hexpointer(buffer->getAddress())
            << " region "    << hexpointer(buffer)
            << " length "    << hexlength(buffer->getLength())
            << " chunksize " << hexlength(small_.chunk_size_) << " " << hexlength(medium_.chunk_size_) << " " << hexlength(large_.chunk_size_)
            << " free (s) "  << decnumber(small_.free_list_.size()) << " used " << decnumber(this->small_.region_use_count_)
            << " free (m) "  << decnumber(medium_.free_list_.size()) << " used " << decnumber(this->medium_.region_use_count_)
            << " free (l) "  << decnumber(large_.free_list_.size()) << " used " << decnumber(this->large_.region_use_count_));
    return buffer;
}

//----------------------------------------------------------------------------
void RdmaMemoryPool::deallocate(RdmaMemoryRegion *region)
{
    // if this region was registered on the fly, then don't return it to the pool
    if (region->isTempRegion() || region->isUserRegion()) {
        if (region->isTempRegion()) {
            temp_regions--;
            LOG_TRACE_MSG("Deallocating temp registered block " << hexpointer(region->getAddress()) << decnumber(temp_regions));
        }
        LOG_DEBUG_MSG("Deleting (user region) " << hexpointer(region));
        delete region;
        return;
    }

    // put the block back on the free list
    if (region->getLength()<=small_.chunk_size_) {
        small_.push(region);
    }
    else if (region->getLength()<=medium_.chunk_size_) {
        medium_.push(region);
    }
    else if (region->getLength()<=large_.chunk_size_) {
        large_.push(region);
    }

    LOG_TRACE_MSG("Pushing Block"
            << " buffer "     << hexpointer(region->getAddress())
            << " region "     << hexpointer(region)
            << " free (s) "  << decnumber(small_.free_list_.size()) << " used " << decnumber(this->small_.region_use_count_)
            << " free (m) "  << decnumber(medium_.free_list_.size()) << " used " << decnumber(this->medium_.region_use_count_)
            << " free (l) "  << decnumber(large_.free_list_.size()) << " used " << decnumber(this->large_.region_use_count_));
}
//----------------------------------------------------------------------------
RdmaMemoryRegionPtr RdmaMemoryPool::AllocateRegisteredBlock(std::size_t length)
{
    LOG_DEBUG_MSG("AllocateRegisteredBlock with this pointer " << hexpointer(this) << " size " << hexlength(length));
    RdmaMemoryRegionPtr region = std::make_shared<RdmaMemoryRegion>();
    region->allocate(protection_domain_, length);
    //
    pointer_map_[region->getAddress()] = region.get();
    return region;
}

//----------------------------------------------------------------------------
RdmaMemoryRegion* RdmaMemoryPool::AllocateTemporaryBlock(std::size_t length)
{
    LOG_DEBUG_MSG("AllocateTemporaryBlock with this pointer " << hexpointer(this) << " size " << hexlength(length));
    RdmaMemoryRegion *region = new RdmaMemoryRegion();
    region->setTempRegion();
    region->allocate(protection_domain_, length);
    temp_regions++;
    LOG_TRACE_MSG("Allocating temp registered block " << hexpointer(region->getAddress()) << hexlength(length) << decnumber(temp_regions));
    return region;
}

//----------------------------------------------------------------------------
int RdmaMemoryPool::DeallocateList()
{
    bool ok = true;
    ok = ok && small_.DeallocatePool();
    ok = ok && medium_.DeallocatePool();
    ok = ok && large_.DeallocatePool();
    return ok;
}

//----------------------------------------------------------------------------
