#include "memory_pool.hpp"
#include "rdmahelper_logging.h"
//
#include <cstdlib>
#include <cstdio>
#include <iomanip>
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
  RdmaMemoryRegionPtr buffer = free_list_.top();
  free_list_.pop();

  H5FDdsmDebugLevel(5,"Popping Block "
      << " region  "   << std::setfill('0') << std::setw(12) << std::hex << (pointer)buffer.get()
      << " buffer  "   << std::setfill('0') << std::setw(12) << std::hex << (pointer)buffer->getAddress()
      << " length  0x" << std::setfill('0') << std::setw(12) << std::hex << length
      << " chunk size 0x" << std::setfill('0') << std::setw(12) << std::hex << chunk_size_
      << " size is " << std::dec << free_list_.size() << " refcount " << this->BufferReferenceCount);
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

   H5FDdsmDebugLevel(5,"Pushing Block "
      << " buffer  " << std::setfill('0') << std::setw(12) << std::hex << (pointer)r->getAddress()
      << " size is " << std::dec << free_list_.size() << " refcount " << this->BufferReferenceCount);
  
  this->memBuffer_cond.notify_one();

  H5FDdsmDebugLevel(5,"notify one called ");
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
  LOG_DEBUG_MSG("AllocateRegisteredBlock for size " << length);
  RdmaMemoryRegionPtr region = std::make_shared<RdmaMemoryRegion>();
#ifndef __BGQ__
  region->allocate(protection_domain_, length);
#else
  region->allocate(rdma_fd_, length);
#endif
  H5FDdsmDebugLevel(2,"Allocating registered block "
              << "0x" << std::setfill('0') << std::setw(12) << std::hex << (pointer)region->getAddress()
      << " length 0x" << std::setfill('0') << std::setw(12) << std::hex << length);

  free_list_.push(region);
  this->BlockList[region.get()] = region;
  H5FDdsmDebugLevel(6,
      "Adding registered block to buffer " << "0x" << std::setfill('0') << std::setw(12) << std::hex << (pointer)region->getAddress());

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
  H5FDdsmDebugLevel(0,
      "Allocating " << std::dec << num_chunks << " blocks of " << "0x" << std::setfill('0') << std::setw(12) << std::hex << this->chunk_size_);
  //
  for (int i = 0; i < num_chunks; i++) {
    RdmaMemoryRegionPtr buffer = this->AllocateRegisteredBlock(this->chunk_size_);
    if (buffer == NULL) {
      this->max_chunks_ = i - 1;
      H5FDdsmError("Block Allocation Stopped at " << (i-1));
      return 0;
    }
 }

  return (1);
}

//----------------------------------------------------------------------------
int memory_pool::DeallocateListBase()
{
  H5FDdsmDebug("Deallocating free_list_");
  // max_chunks_;
  if (free_list_.size()!=BlockList.size() || this->BufferReferenceCount!=0) {
    H5FDdsmError("Deallocating free_list_ : Not all blocks were returned "
        << free_list_.size() << " instead of " << BlockList.size() << " refcounts " << this->BufferReferenceCount);
  }
  else {
    H5FDdsmDebug("Deallocating free_list_, OK");
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
    RdmaMemoryRegionPtr buffer = free_list_.top();
    free_list_.pop();
//    struct ibv_mr *mr = this->BlockRegistrationMap[buffer];
//    if (ibv_dereg_mr(mr)) {
//      LOG_ERROR_MSG("ibv_dereg_mr() failed in NVP::Deallocate");
//    }
//    free(buffer);
  }
  return(1);
}

