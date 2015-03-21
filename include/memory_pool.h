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
#ifndef __memory_pool_hpp_
#define __memory_pool_hpp_

//
//#define HPX_CONDITION
//

//
#ifdef HPX_CONDITION
 #include <hpx/lcos/local/spinlock.hpp>
 #include <hpx/lcos/local/recursive_mutex.hpp>
 #include <hpx/lcos/local/condition_variable.hpp>
#endif
//
#include <atomic>
#include <stack>
#include <queue>
#include <mutex>
#include <vector>
#include <map>
#include <condition_variable>
#include <iostream>
//
#include <boost/noncopyable.hpp>
#ifndef __BGQ__
  #include "RdmaProtectionDomain.h"
  #include "RdmaMemoryRegion.h"
#else
  #include "CNKMemoryRegion.h"
#endif

#define pointer uintptr_t

using namespace bgcios;

struct memory_pool : boost::noncopyable
{
  typedef std::size_t size_type;
  //  typedef std::multimap<size_type, char *> large_chunks_type;

#ifndef __BGQ__
  memory_pool(RdmaProtectionDomainPtr pd, std::size_t chunk_size, std::size_t init_chunks, std::size_t max_chunks) :
    protection_domain_(pd) ,
#else
  memory_pool(int rdma_fd, std::size_t chunk_size, std::size_t init_chunks, std::size_t max_chunks) :
  rdma_fd_(rdma_fd) ,
#endif
    isServer(true)
  , chunk_size_(chunk_size)
  , max_chunks_(max_chunks)
  , BufferReferenceCount(0)
  {
    AllocateList(init_chunks);
  }

  ~memory_pool();

#ifndef __BGQ__
  void setProtectionDomain(RdmaProtectionDomainPtr pd) {
    DeallocateList();
    protection_domain_ = pd;
    AllocateList();
  }
#else
  void setProtectionDomain(int rdma_fd) {
    DeallocateList();
    rdma_fd_ = rdma_fd;
    AllocateList();
  }
#endif

   int   AllocateList(std::size_t chunks=0);
   int   DeallocateList();
   int   DeallocateListBase();

   RdmaMemoryRegionPtr AllocateRegisteredBlock(int length);

   RdmaMemoryRegion *allocate(size_t size=0);
   void              deallocate(RdmaMemoryRegion *region);

#ifdef HPX_CONDITION
  typedef hpx::lcos::local::spinlock              mutex_type;
  typedef hpx::lcos::local::spinlock::scoped_lock lock_type1;
  typedef hpx::lcos::local::spinlock::scoped_lock lock_type2;
  typedef hpx::lcos::local::condition_variable    condition_type;
#else
  typedef std::mutex                    mutex_type;
  typedef std::lock_guard<std::mutex>   lock_type1;
  typedef std::unique_lock<std::mutex>  lock_type2;
  typedef std::condition_variable       condition_type;
#endif
  //
  //
  static const uint32_t H5FDdsm_VIRTUAL_DATA_ALIGNMENT = 65536;
  //
  std::map<RdmaMemoryRegion*, RdmaMemoryRegionPtr> BlockList;
  std::queue<RdmaMemoryRegionPtr>                  free_list_;
  //
  // Referencing for memory blocks and associated registered memory struct
//  typedef std::pair<RdmaMemoryRegion*, struct ibv_mr *> memoryPair;
//  std::map<RdmaMemoryRegion*, struct ibv_mr *> BlockRegistrationMap;

  memory_pool::mutex_type     memBuffer_mutex;
  memory_pool::condition_type memBuffer_cond;

#ifndef __BGQ__
  RdmaProtectionDomainPtr    protection_domain_;
#else
  int rdma_fd_;
#endif
  bool                       isServer;
  std::size_t                chunk_size_;
  std::size_t                max_chunks_;
  std::atomic<int>           BufferReferenceCount;
  mutable mutex_type         large_chunks_mtx_;
  //large_chunks_type          large_chunks_;
  //  std::vector<memory_chunk>  memory_chunks_;
//  boost::atomic<std::size_t> RdmaMemoryRegionPtrged_chunks_;
 };

typedef std::shared_ptr<memory_pool> memory_poolPtr;

#endif
