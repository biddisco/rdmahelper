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
//
//----------------------------------------------------------------------------
RdmaMemoryPool::~RdmaMemoryPool()
{
  DeallocateList();
}
//----------------------------------------------------------------------------
char *RdmaMemoryPool::allocate(size_t length)
{
    if (length>chunk_size_) {
        LOG_ERROR_MSG("Chunk pool size exceeded " << length);
        std::terminate();
        throw pinned_memory_exception(std::string(std::string("Chunk pool size exceeded ") + std::to_string(length)).c_str());
    }
    RdmaMemoryRegion *region = allocateRegion(length);
    return static_cast<char*>(region->getAddress());
}

//----------------------------------------------------------------------------
RdmaMemoryRegion *RdmaMemoryPool::allocateRegion(size_t length)
{
  //LOG_DEBUG_MSG("allocate region with this pointer " << hexpointer(this))
  // we must protect our queue from thread contention
  scoped_lock lock(memBuffer_mutex);

  // if we have not exceeded our max size, allocate a new block
  if (free_list_.empty() && block_list_.size()<max_chunks_) {
    LOG_TRACE_MSG("Creating new Block as free list is empty but max chunks " << max_chunks_ << " not reached");
    AllocateRegisteredBlock(length);
  }
  // make sure the list is not empty, wait on condition
  this->memBuffer_cond.wait(lock, [this] {
    return !free_list_.empty();
  });

  // Keep reference counts to self so that this pool is not deleted whilst blocks still exist
  this->region_ref_count_++;

  // get a block
  RdmaMemoryRegion *buffer = free_list_.top();
  free_list_.pop();

  LOG_TRACE_MSG("Popping Block"
      << " region "    << hexpointer(buffer)
      << " buffer "    << hexpointer(buffer->getAddress())
      << " length "    << hexlength(length)
      << " chunksize " << hexlength(chunk_size_)
      << " free "      << decnumber(free_list_.size()) << " used " << decnumber(this->region_ref_count_));
  if (length>this->chunk_size_) {
    throw std::runtime_error("Fatal, block size too small");
  }
  return buffer;
}

//----------------------------------------------------------------------------
void RdmaMemoryPool::deallocate(void *address, size_t size)
{
  scoped_lock lock(memBuffer_mutex);
  RdmaMemoryRegion *region = pointer_map_[address];
  deallocate(region);
}

//----------------------------------------------------------------------------
void RdmaMemoryPool::deallocate(RdmaMemoryRegion *region)
{
  // we must protect our mem buffer from thread contention
  unique_lock lock(memBuffer_mutex);

  // put the block back on the free list
  free_list_.push(region);

  // decrement one reference
  this->region_ref_count_--;

  LOG_TRACE_MSG("Pushing Block"
      << " region " << hexpointer(region)
      << " buffer " << hexpointer(region->getAddress())
      << " free "   << decnumber(free_list_.size()) << " used " << decnumber(this->region_ref_count_));

  lock.unlock();
  // if anyone was waiting on the free list lock, then give it
  this->memBuffer_cond.notify_one();
  LOG_TRACE_MSG("notify one called 1");
}
//----------------------------------------------------------------------------
RdmaMemoryRegion* RdmaMemoryPool::AllocateRegisteredBlock(int length)
{
  LOG_DEBUG_MSG("AllocateRegisteredBlock with this pointer " << hexpointer(this) << " size " << hexlength(length));
  RdmaMemoryRegionPtr region = std::make_shared<RdmaMemoryRegion>();
#ifndef __BGQ__
  region->allocate(protection_domain_, length);
#else
  region->allocate(rdma_fd_, length);
#endif
  LOG_TRACE_MSG("Allocating registered block " << hexpointer(region->getAddress()) << hexlength(length));
  unique_lock lock(memBuffer_mutex);

  free_list_.push(region.get());
  block_list_[region.get()] = region;
  pointer_map_[region->getAddress()] = region.get();
  LOG_TRACE_MSG("Adding registered block to buffer " << hexpointer(region->getAddress()));

  lock.unlock();
  // if anyone was waiting on the free list lock, then give it
  this->memBuffer_cond.notify_one();
  LOG_TRACE_MSG("notify one called 2");

  return region.get();
}

//----------------------------------------------------------------------------
RdmaMemoryRegion* RdmaMemoryPool::AllocateTemporaryBlock(int length)
{
  LOG_DEBUG_MSG("AllocateTemporaryBlock with this pointer " << hexpointer(this) << " size " << hexlength(length));
  RdmaMemoryRegion *region = new RdmaMemoryRegion();
  region->setTempRegion();
#ifndef __BGQ__
  region->allocate(protection_domain_, length);
#else
  region->allocate(rdma_fd_, length);
#endif
  LOG_TRACE_MSG("Allocating registered block " << hexpointer(region->getAddress()) << hexlength(length));

  return region;
}

//----------------------------------------------------------------------------
int RdmaMemoryPool::AllocateList(std::size_t chunks)
{
  std::size_t num_chunks = chunks==0 ? max_chunks_ : chunks;
  //
  // Pre-Allocate temporary blocks of RAM for transient put/get operations
  //
  LOG_DEBUG_MSG("Allocating " << std::dec << num_chunks << " blocks of " << hexlength(this->chunk_size_));
  //
  for (std::size_t i=0; i<num_chunks; i++) {
    RdmaMemoryRegion *buffer = this->AllocateRegisteredBlock(this->chunk_size_);
    if (buffer == NULL) {
      this->max_chunks_ = i - 1;
      LOG_ERROR_MSG("Block Allocation Stopped at " << (i-1));
      return 0;
    }
 }

  return (1);
}

//----------------------------------------------------------------------------
int RdmaMemoryPool::DeallocateListBase()
{
  // max_chunks_;
  if (free_list_.size()!=block_list_.size() || this->region_ref_count_!=0) {
    LOG_ERROR_MSG("Deallocating free_list_ : Not all blocks were returned "
        << decnumber(free_list_.size()) << " instead of " << decnumber(block_list_.size() )<< " refcounts " << decnumber(this->region_ref_count_));
  }
  else {
    LOG_DEBUG_MSG("Free list deallocation OK");
    this->max_chunks_ = 0;
  }
  block_list_.clear();
  return(1);
}

//----------------------------------------------------------------------------
int RdmaMemoryPool::DeallocateList()
{
//  if (this->CompletionHandlerStatus != completed) {
//   throw std::runtime_error("Asynchronous service must be stopped before calling deallocate");
//  }

//  if (this->NumNVPRequestsPending>0) {
//    std::cout << "NumNVPRequestsPending " << NumNVPRequestsPending << std::endl;
//    throw std::runtime_error("Requests are still pending when entering deallocate");
//  }

  // let base class do some checks
  DeallocateListBase();

  while (!free_list_.empty()) {
//    RdmaMemoryRegion *buffer = free_list_.front();
    free_list_.pop();
//    struct ibv_mr *mr = this->BlockRegistrationMap[buffer];
//    if (ibv_dereg_mr(mr)) {
//      LOG_ERROR_MSG("ibv_dereg_mr() failed in NVP::Deallocate");
//    }
//    free(buffer);
  }
  return(1);
}

