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
#ifndef __RdmaMemoryPool_hpp_
#define __RdmaMemoryPool_hpp_

#ifdef HPX_PARCELPORT_VERBS
# ifndef RDMAHELPER_HPX_COMPATIBILITY
#  define RDMAHELPER_HPX_COMPATIBILITY
# endif
#endif

#ifdef RDMAHELPER_HPX_COMPATIBILITY
 #include <hpx/lcos/local/spinlock.hpp>
 #include <hpx/lcos/local/recursive_mutex.hpp>
 #include <hpx/lcos/local/condition_variable.hpp>
 #include <hpx/traits/is_chunk_allocator.hpp>
 #include <hpx/util/memory_chunk_pool_allocator.hpp>
#else
 #include <mutex>
 #include <condition_variable>
#endif
//
#include <atomic>
#include <queue>
#include <vector>
#include <map>
#include <unordered_map>
#include <iostream>
//
#include <boost/noncopyable.hpp>
#ifndef __BGQ__
  #include "RdmaProtectionDomain.h"
  #include "RdmaMemoryRegion.h"
#else
  #include "CNKMemoryRegion.h"
#endif

// if the HPX configuration has set a default chunk size, use it
#if defined(HPX_PARCELPORT_VERBS_MEMORY_CHUNK_SIZE)
# define DEFAULT_MEMORY_POOL_CHUNK_SIZE HPX_PARCELPORT_VERBS_MEMORY_CHUNK_SIZE
#else
// deliberately small to trigger exceptions whilst debugging
# define DEFAULT_MEMORY_POOL_CHUNK_SIZE 256
#endif

class pinned_memory_exception : public std::runtime_error
{
public:
  pinned_memory_exception(const char *msg) : std::runtime_error(msg) {};
};

using namespace bgcios;

struct RdmaMemoryPool;

#ifdef RDMAHELPER_HPX_COMPATIBILITY
// specialize chunk pool allocator traits for this memory_chunk_pool
namespace hpx { namespace traits
{
    // if the chunk pool supplies fixed chunks of memory when the alloc
    // is smaller than some threshold, then the pool must declare
    // std::size_t chunk_size_
    template <typename T, typename M>
    struct is_chunk_allocator<util::detail::memory_chunk_pool_allocator<T,RdmaMemoryPool,M>>
      : std::true_type {};
}}
#endif

//template <typename Mutex = hpx::lcos::local::spinlock>
struct RdmaMemoryPool : boost::noncopyable
{
  typedef std::size_t size_type;
  typedef char        value_type;

#ifndef __BGQ__
  RdmaMemoryPool(RdmaProtectionDomainPtr pd, std::size_t chunk_size, std::size_t init_chunks, std::size_t max_chunks) :
    protection_domain_(pd) ,
#else
  RdmaMemoryPool(int rdma_fd, std::size_t chunk_size, std::size_t init_chunks, std::size_t max_chunks) :
  rdma_fd_(rdma_fd) ,
#endif
    isServer(true)
  , chunk_size_(chunk_size)
  , max_chunks_(max_chunks)
  , region_ref_count_(0)
  {
    AllocateList(init_chunks);
  }

  ~RdmaMemoryPool();

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

   RdmaMemoryRegion *AllocateRegisteredBlock(int length);
   RdmaMemoryRegion *AllocateTemporaryBlock(int length);

   RdmaMemoryRegion *allocateRegion(size_t size=0);
   char             *allocate(size_t size=0);
   void              deallocate(void *address, size_t size=0);
   void              deallocate(RdmaMemoryRegion *region);

   RdmaMemoryRegion *RegionFromAddress(char * const addr) {
     return pointer_map_[addr];
   }

   inline size_t default_chunk_size() { return chunk_size_; }

//   void construct(typename std::allocator<T>::pointer p){
//     new ((void*)p) T(47);
//   }

#ifdef RDMAHELPER_HPX_COMPATIBILITY
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

  // not used any more, but keep for a while ...
  static const uint32_t mempool_VIRTUAL_DATA_ALIGNMENT = 65536;

  // every block we allocate will be added to this map so that
  // we hold a reference count to them and prevent their deletion
  // this list is only used at start and finish
  std::map<RdmaMemoryRegion*, RdmaMemoryRegionPtr> block_list_;

  // blocks that have not been allocated are available from here
  std::queue<RdmaMemoryRegion*>                    free_list_;

  // used to map the internal memory address to the region that
  // holds the registration information
  std::unordered_map<const void *, RdmaMemoryRegion*> pointer_map_;

  mutex_type     memBuffer_mutex;
  condition_type memBuffer_cond;

#ifndef __BGQ__
  RdmaProtectionDomainPtr    protection_domain_;
#else
  int rdma_fd_;
#endif
  bool                       isServer;
  std::size_t                chunk_size_;
  std::size_t                max_chunks_;
  std::atomic<int>           region_ref_count_;
  mutable mutex_type         large_chunks_mtx_;
 };

typedef std::shared_ptr<RdmaMemoryPool> RdmaMemoryPoolPtr;

#endif
