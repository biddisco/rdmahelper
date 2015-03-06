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
#include "memory_pool.h"
#include "RdmaLogging.h"
//
#include <cstdlib>
#include <cstdio>
#include <iomanip>
//
//----------------------------------------------------------------------------
memory_pool::~memory_pool()
{
  DeallocateList();
}
//----------------------------------------------------------------------------
RdmaMemoryRegionPtr memory_pool::allocate(size_t length)
{
  // we must protect our queue from thread contention
  lock_type2 lock(memBuffer_mutex);

  // if we have not exceeded our max size, allocate a new block
  if (free_list_.empty() && BlockList.size()<max_chunks_) {
    LOG_TRACE_MSG("Creating new Block as free list is empty but max chunks " << max_chunks_ << " not reached");
    AllocateRegisteredBlock(length);
  }
  // make sure the deque is not empty, wait on condition
  this->memBuffer_cond.wait(lock, [this] {
    return !free_list_.empty();
  });

  // Each block allocated may be released by the hpx garbage collection at some undetermined point, 
  // so keep reference counts to self so that when the blocks are freed we don't get a segfault.
  this->BufferReferenceCount++;

  // get a block
  RdmaMemoryRegionPtr buffer = free_list_.front();
  free_list_.pop();

  LOG_TRACE_MSG("Popping Block"
      << " region "    << hexpointer(buffer.get())
      << " buffer "    << hexpointer(buffer->getAddress())
      << " length "    << hexlength(length)
      << " chunksize " << hexlength(chunk_size_)
      << " size is   " << std::dec << free_list_.size() << " refcount " << this->BufferReferenceCount);
  if (length>this->chunk_size_) {
    throw std::runtime_error("Fatal, block size too small");
  }
  return buffer;
}

//----------------------------------------------------------------------------
void memory_pool::deallocate(RdmaMemoryRegion *region)
{
  // we must protect our mem buffer from thread contention
  lock_type2 lock(memBuffer_mutex);

  RdmaMemoryRegionPtr r = BlockList[region];
  // put the block back on the stack
  free_list_.push(r);

  // release one reference
  this->BufferReferenceCount--;

  LOG_TRACE_MSG("Pushing Block"
      << " region  " << hexpointer(region)
      << " buffer  " << hexpointer(r->getAddress())
      << " size is " << std::dec << free_list_.size() << " refcount " << this->BufferReferenceCount);
  
  this->memBuffer_cond.notify_one();

  LOG_TRACE_MSG("notify one called ");
  return;
}
//----------------------------------------------------------------------------
bool memory_pool::WaitForCompletion()
{
  while (free_list_.size()<this->max_chunks_) {
//    hpx::this_thread::suspend(boost::posix_time::microseconds(5));
  }
  return 1;
}
//----------------------------------------------------------------------------
RdmaMemoryRegionPtr memory_pool::AllocateRegisteredBlock(int length)
{
  LOG_DEBUG_MSG("AllocateRegisteredBlock for size " << hexlength(length));
  RdmaMemoryRegionPtr region = std::make_shared<RdmaMemoryRegion>();
#ifndef __BGQ__
  region->allocate(protection_domain_, length);
#else
  region->allocate(rdma_fd_, length);
#endif
  LOG_TRACE_MSG("Allocating registered block " << hexpointer(region->getAddress()) << hexlength(length));

  free_list_.push(region);
  this->BlockList[region.get()] = region;
  LOG_TRACE_MSG("Adding registered block to buffer " << hexpointer(region->getAddress()));

  // if anyone was waiting on the free list lock, then give it
  this->memBuffer_cond.notify_one();

  //
  // save the memory registration info
  //
//  this->BlockRegistrationMap[region.get()] = region;
  return region;
}

//----------------------------------------------------------------------------
int memory_pool::AllocateList(std::size_t chunks)
{
  std::size_t num_chunks = chunks==0 ? max_chunks_ : chunks;
  //
  // Pre-Allocate temporary blocks of RAM for transient put/get operations
  //
  LOG_DEBUG_MSG("Allocating " << std::dec << num_chunks << " blocks of " << hexlength(this->chunk_size_));
  //
  for (std::size_t i=0; i<num_chunks; i++) {
    RdmaMemoryRegionPtr buffer = this->AllocateRegisteredBlock(this->chunk_size_);
    if (buffer == NULL) {
      this->max_chunks_ = i - 1;
      LOG_ERROR_MSG("Block Allocation Stopped at " << (i-1));
      return 0;
    }
 }

  return (1);
}

//----------------------------------------------------------------------------
int memory_pool::DeallocateListBase()
{
  // max_chunks_;
  if (free_list_.size()!=BlockList.size() || this->BufferReferenceCount!=0) {
    LOG_ERROR_MSG("Deallocating free_list_ : Not all blocks were returned "
        << free_list_.size() << " instead of " << BlockList.size() << " refcounts " << this->BufferReferenceCount);
  }
  else {
    LOG_DEBUG_MSG("Free list deallocation OK");
    this->max_chunks_ = 0;
  }
  BlockList.clear();
  return(1);
}

//----------------------------------------------------------------------------
int memory_pool::DeallocateList()
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
    RdmaMemoryRegionPtr buffer = free_list_.front();
    free_list_.pop();
//    struct ibv_mr *mr = this->BlockRegistrationMap[buffer];
//    if (ibv_dereg_mr(mr)) {
//      LOG_ERROR_MSG("ibv_dereg_mr() failed in NVP::Deallocate");
//    }
//    free(buffer);
  }
  return(1);
}

