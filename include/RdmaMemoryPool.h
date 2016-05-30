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

#ifdef HPX_HAVE_PARCELPORT_VERBS
# ifndef RDMAHELPER_HAVE_HPX
#  define RDMAHELPER_HAVE_HPX
# endif
#endif

#ifdef RDMAHELPER_HAVE_HPX
 #include <hpx/lcos/local/mutex.hpp>
 #include <hpx/lcos/local/condition_variable.hpp>
 #include <hpx/traits/is_chunk_allocator.hpp>
 #include <hpx/util/memory_chunk_pool_allocator.hpp>
#else
 #include <mutex>
 #include <condition_variable>
#endif
//
#include <atomic>
#include <stack>
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

#include "RdmaLogging.h"

// the default memory chunk size in bytes
#define RDMA_DEFAULT_MEMORY_POOL_SMALL_CHUNK_SIZE   4096
#define RDMA_DEFAULT_MEMORY_POOL_MEDIUM_CHUNK_SIZE 16384
#define RDMA_DEFAULT_MEMORY_POOL_LARGE_CHUNK_SIZE  1048576
// the default number of chunks we allocate with our pool
#define RDMA_DEFAULT_CHUNKS_ALLOC 32
// the maximum number of chunks we can allocate with our pool
#define RDMA_MAX_CHUNKS_ALLOC 64
// the maximum number of preposted receives
#define RDMA_MAX_PREPOSTS 32

#define RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS  RDMA_MAX_CHUNKS_ALLOC
#define RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS 16
#define RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS  4

// if the HPX configuration has set a different value, use it
#if defined(HPX_PARCELPORT_VERBS_MEMORY_CHUNK_SIZE)
# undef RDMA_DEFAULT_MEMORY_POOL_SMALL_CHUNK_SIZE
# define RDMA_DEFAULT_MEMORY_POOL_SMALL_CHUNK_SIZE HPX_PARCELPORT_VERBS_MEMORY_CHUNK_SIZE
#endif

// if the HPX configuration has set a different value, use it
#if defined(HPX_PARCELPORT_VERBS_DEFAULT_MEMORY_CHUNKS)
# undef RDMA_DEFAULT_CHUNKS_ALLOC
# define RDMA_DEFAULT_CHUNKS_ALLOC HPX_PARCELPORT_VERBS_DEFAULT_MEMORY_CHUNKS
#endif

// if the HPX configuration has set a different value, use it
#if defined(HPX_PARCELPORT_VERBS_MAX_MEMORY_CHUNKS)
# undef RDMA_MAX_CHUNKS_ALLOC
# define RDMA_MAX_CHUNKS_ALLOC HPX_PARCELPORT_VERBS_MAX_MEMORY_CHUNKS
#endif

// if the HPX configuration has set a different value, use it
#if defined(HPX_PARCELPORT_VERBS_MAX_PREPOSTS)
# undef RDMA_MAX_PREPOSTS
# define RDMA_MAX_PREPOSTS HPX_PARCELPORT_VERBS_MAX_PREPOSTS
#endif

// -------------------------
// utility exception class
// -------------------------
class pinned_memory_exception : public std::runtime_error
{
public:
  pinned_memory_exception(const char *msg) : std::runtime_error(msg) {};
};

using namespace bgcios;

struct RdmaMemoryPool;

#ifdef RDMAHELPER_HAVE_HPX
// -------------------------
// specialize chunk pool allocator traits for this memory_chunk_pool
// -------------------------
namespace hpx { namespace traits
{
    // if the chunk pool supplies fixed chunks of memory when the alloc
    // is smaller than some threshold, then the pool must declare
    // std::size_t small_chunk_size_
    template <typename T, typename M>
    struct is_chunk_allocator<util::detail::memory_chunk_pool_allocator<T,RdmaMemoryPool,M>>
      : std::false_type {};

    template <>
    struct is_chunk_allocator<RdmaMemoryPool>
      : std::false_type {};
}}
#endif

// ---------------------------------------------------------------------------
// pool_container, collect some routines for reuse with
// small, medium, large chunks
// ---------------------------------------------------------------------------
struct pool_container
{
    typedef std::function<RdmaMemoryRegionPtr(std::size_t)> regionAllocFunction;
#ifdef RDMAHELPER_HAVE_HPX
    typedef hpx::lcos::local::mutex                   mutex_type;
    typedef std::lock_guard<mutex_type>               scoped_lock;
    typedef std::unique_lock<mutex_type>              unique_lock;
    typedef hpx::lcos::local::condition_variable      condition_type;
#else
    typedef std::mutex                    mutex_type;
    typedef std::lock_guard<std::mutex>   scoped_lock;
    typedef std::condition_variable       condition_type;
#endif

    // ------------------------------------------------------------------------
    pool_container(std::size_t chunk_size, std::size_t items) :
        chunk_size_(chunk_size), max_chunks_(items), region_use_count_(0)
    {
        LOG_DEBUG_MSG("Creating pool with chunk_size " << hexnumber(chunk_size_)
                << "max_chunks " << max_chunks_);
    }

    // ------------------------------------------------------------------------
    bool AllocatePool(std::size_t _num_chunks, regionAllocFunction f)
    {
        LOG_DEBUG_MSG("Allocating " << std::dec << _num_chunks << " blocks of " << hexlength(chunk_size_));
        //
        for (std::size_t i=0; i<_num_chunks; i++) {
            LOG_DEBUG_MSG("Allocate Block " << i << " of size " << hexlength(chunk_size_));
            RdmaMemoryRegionPtr region = f(chunk_size_);
            if (region!=nullptr) {
                block_list_[region.get()] = region;
                push(region.get());
            }
            else {
                LOG_ERROR_MSG("Block Allocation Stopped at " << (i-1));
                return false;
            }
        }
        region_use_count_ = 0;
        return true;
    }

    // ------------------------------------------------------------------------
    int DeallocatePool()
    {
        if (free_list_.size()!=block_list_.size() || region_use_count_!=0) {
            LOG_ERROR_MSG("Deallocating free_list : Not all blocks were returned "
                    << decnumber(free_list_.size())
                    << " instead of " << decnumber(block_list_.size())
                    << " refcounts " << decnumber(region_use_count_));
            return 0;
        }
        while (!free_list_.empty()) {
            pop();
        }
        // release shared pointer
        block_list_.clear();
        return 1;
    }

    // ------------------------------------------------------------------------
    void push(RdmaMemoryRegion *region) 
    {
        LOG_TRACE_MSG("Push block " << hexpointer(region->getAddress()) << hexlength(region->getLength()));
        // we must protect our mem buffer from thread contention
        unique_lock lock(memBuffer_mutex_);
        free_list_.push(region);
        // decrement one reference
        region_use_count_--;
        lock.unlock();
        // if anyone was waiting on the free list lock, then give it
        memBuffer_cond_.notify_one();
    }

    RdmaMemoryRegion *pop() 
    {
        unique_lock lock(memBuffer_mutex_);
        // if we have not exceeded our max size, allocate a new block
        if (free_list_.empty() && block_list_.size()<max_chunks_) {
            // LOG_TRACE_MSG("Creating new small Block as free list is empty but max chunks " << max_small_chunks_ << " not reached");
            //  AllocateRegisteredBlock(length);
        }
        // make sure the list is not empty, wait on condition
        memBuffer_cond_.wait(lock, [this] {
            return !free_list_.empty();
        });
        // get a block
        RdmaMemoryRegion *region = free_list_.top();
        free_list_.pop();
        // Keep reference counts to self so that we can check
        // this pool is not deleted whilst blocks still exist
        region_use_count_++;
        LOG_TRACE_MSG("Pop block " << hexpointer(region->getAddress()) << hexlength(region->getLength()));
        return region;
    }

    //
    std::size_t                     chunk_size_;
    std::size_t                     max_chunks_;
    std::atomic<int>                region_use_count_;
    std::stack<RdmaMemoryRegion*>   free_list_;
    mutex_type                      memBuffer_mutex_;
    condition_type                  memBuffer_cond_;
    std::unordered_map<RdmaMemoryRegion*, RdmaMemoryRegionPtr> block_list_;
};

// ---------------------------------------------------------------------------
// memory pool is a type of std:: compatible allocator
// ---------------------------------------------------------------------------
struct RdmaMemoryPool : boost::noncopyable
{
    typedef std::size_t size_type;
    typedef char        value_type;

#ifdef RDMAHELPER_HAVE_HPX
    typedef hpx::lcos::local::mutex                   mutex_type;
    typedef std::lock_guard<mutex_type>               scoped_lock;
    typedef std::unique_lock<mutex_type>              unique_lock;
    typedef hpx::lcos::local::condition_variable      condition_type;
#else
    typedef std::mutex                    mutex_type;
    typedef std::lock_guard<std::mutex>   scoped_lock;
    typedef std::condition_variable       condition_type;
#endif

#ifdef __BGQ__
    typedef int protection_domain_type;
#else
    typedef RdmaProtectionDomainPtr protection_domain_type;
#endif

    // constructor
    RdmaMemoryPool(protection_domain_type pd, std::size_t chunk_size, std::size_t init_chunks, std::size_t max_chunks);

    // destructor
    ~RdmaMemoryPool();

    // set the protection domain, triggers pool deallocation and reallocation
    void setProtectionDomain(protection_domain_type pd);

    int   DeallocateList();
    int   DeallocateListBase();

    RdmaMemoryRegionPtr AllocateRegisteredBlock(std::size_t length);
    RdmaMemoryRegion   *AllocateTemporaryBlock(std::size_t length);

    // -------------------------
    // User allocation interface
    // -------------------------
    // The RdmaMemoryRegion* versions of allocate/deallocate
    // should be used in preference to the std:: compatible
    // versions using char* for efficiency

    // allocate a region, if size=0 a small region is returned
    RdmaMemoryRegion *allocateRegion(size_t size);

    // release a region back to the pool
    void deallocate(RdmaMemoryRegion *region);

    // allocate a region, returning a memory block address
    char *allocate(size_t size);

    // deallocate a region using its memory address as handle
    // this involves a map lookup to find the region and is therefore
    // less efficient than releasing memory via the region pointer
    void deallocate(void *address, size_t size=0)
    {
        RdmaMemoryRegion *region = pointer_map_[address];
        deallocate(region);
    }

    // find an RdmaMemoryRegion* from the memory address it wraps
    RdmaMemoryRegion *RegionFromAddress(char * const addr) {
        return pointer_map_[addr];
    }

    // utility functions
//    std::size_t small_chunk_size()  { return small_.chunk_size_; }
//    std::size_t medium_chunk_size() { return medium_.chunk_size_; }
//    std::size_t large_chunk_size()  { return large_.chunk_size_; }

    // used to map the internal memory address to the region that
    // holds the registration information
    std::unordered_map<const void *, RdmaMemoryRegion*> pointer_map_;

    protection_domain_type    protection_domain_;
    bool                      isServer;

    // maintain 3 pools of pre-allocated regions
    pool_container small_;
    pool_container medium_;
    pool_container large_;

};

typedef std::shared_ptr<RdmaMemoryPool> RdmaMemoryPoolPtr;

#endif
