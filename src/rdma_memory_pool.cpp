//  Copyright (c) 2015-2016 John Biddiscombe
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_memory_pool.hpp>
#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <functional>
//
//----------------------------------------------------------------------------
rdma_memory_pool::rdma_memory_pool(protection_domain_type pd, std::size_t chunk_size,
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
            std::bind(&rdma_memory_pool::AllocateRegisteredBlock, this, std::placeholders::_1);
    small_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS, alloc);
    medium_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS, alloc);
    large_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS, alloc);
}
//----------------------------------------------------------------------------
rdma_memory_pool::~rdma_memory_pool()
{
    DeallocateList();
}
//----------------------------------------------------------------------------
void rdma_memory_pool::setProtectionDomain(protection_domain_type pd)
{
    DeallocateList();
    protection_domain_ = pd;
    pool_container::regionAllocFunction alloc =
            std::bind(&rdma_memory_pool::AllocateRegisteredBlock, this, std::placeholders::_1);
    small_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS, alloc);
    medium_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS, alloc);
    large_.AllocatePool(RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS, alloc);
}
//----------------------------------------------------------------------------
char *rdma_memory_pool::allocate(size_t length)
{
    if (length>large_.chunk_size_) {
        LOG_ERROR_MSG("Chunk pool size exceeded " << length);
        std::terminate();
        throw pinned_memory_exception(std::string(std::string("Chunk pool size exceeded ") + std::to_string(length)).c_str());
    }
    rdma_memory_region *region = allocateRegion(length);
    return static_cast<char*>(region->getAddress());
}

//----------------------------------------------------------------------------
bool rdma_memory_pool::canAllocateRegionUnsafe(size_t length)
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
rdma_memory_region *rdma_memory_pool::allocateRegion(size_t length)
{
    rdma_memory_region *buffer;
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
void rdma_memory_pool::deallocate(rdma_memory_region *region)
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
rdma_memory_regionPtr rdma_memory_pool::AllocateRegisteredBlock(std::size_t length)
{
    LOG_DEBUG_MSG("AllocateRegisteredBlock with this pointer " << hexpointer(this) << " size " << hexlength(length));
    rdma_memory_regionPtr region = std::make_shared<rdma_memory_region>();
    region->allocate(protection_domain_, length);
    //
    pointer_map_[region->getAddress()] = region.get();
    return region;
}

//----------------------------------------------------------------------------
rdma_memory_region* rdma_memory_pool::AllocateTemporaryBlock(std::size_t length)
{
    LOG_DEBUG_MSG("AllocateTemporaryBlock with this pointer " << hexpointer(this) << " size " << hexlength(length));
    rdma_memory_region *region = new rdma_memory_region();
    region->setTempRegion();
    region->allocate(protection_domain_, length);
    temp_regions++;
    LOG_TRACE_MSG("Allocating temp registered block " << hexpointer(region->getAddress()) << hexlength(length) << decnumber(temp_regions));
    return region;
}

//----------------------------------------------------------------------------
int rdma_memory_pool::DeallocateList()
{
    bool ok = true;
    ok = ok && small_.DeallocatePool();
    ok = ok && medium_.DeallocatePool();
    ok = ok && large_.DeallocatePool();
    return ok;
}

//----------------------------------------------------------------------------
