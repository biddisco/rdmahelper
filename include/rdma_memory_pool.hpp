//  Copyright (c) 2014-2016 John Biddiscombe
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_VERBS_MEMORY_POOL
#define HPX_PARCELSET_POLICIES_VERBS_MEMORY_POOL
//
#include <hpx/lcos/local/mutex.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/lcos/local/condition_variable.hpp>
#include <hpx/traits/is_chunk_allocator.hpp>
#include <hpx/util/memory_chunk_pool_allocator.hpp>
//
#include <atomic>
#include <stack>
#include <unordered_map>
#include <iostream>
//
#include <boost/noncopyable.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_protection_domain.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_memory_region.hpp>

// the default memory chunk size in bytes
#define RDMA_DEFAULT_MEMORY_POOL_SMALL_CHUNK_SIZE  0x001000 // 4KB
#define RDMA_DEFAULT_MEMORY_POOL_MEDIUM_CHUNK_SIZE 0x004000 // 16KB
#define RDMA_DEFAULT_MEMORY_POOL_LARGE_CHUNK_SIZE  0x100000 // 1MB

// the default number of chunks we allocate with our pool
#define RDMA_DEFAULT_CHUNKS_ALLOC 256
// the maximum number of chunks we can allocate with our pool
#define RDMA_MAX_CHUNKS_ALLOC 256
// the maximum number of preposted receives
#define RDMA_MAX_PREPOSTS 8

#define RDMA_DEFAULT_MEMORY_POOL_MAX_SMALL_CHUNKS  RDMA_MAX_CHUNKS_ALLOC
#define RDMA_DEFAULT_MEMORY_POOL_MAX_MEDIUM_CHUNKS 64
#define RDMA_DEFAULT_MEMORY_POOL_MAX_LARGE_CHUNKS  16

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

using namespace bgcios;
using namespace hpx::parcelset::policies::verbs;

// -------------------------
// utility exception class
// -------------------------
class pinned_memory_exception : public std::runtime_error
{
public:
  pinned_memory_exception(const char *msg) : std::runtime_error(msg) {};
};

using namespace bgcios;

struct rdma_memory_pool;

#ifdef RDMA_HANDLER_HAVE_HPX
// -------------------------
// specialize chunk pool allocator traits for this memory_chunk_pool
// -------------------------
namespace hpx { namespace traits
{
    // if the chunk pool supplies fixed chunks of memory when the alloc
    // is smaller than some threshold, then the pool must declare
    // std::size_t small_chunk_size_
    template <typename T, typename M>
    struct is_chunk_allocator<util::detail::memory_chunk_pool_allocator<T,rdma_memory_pool,M>>
      : std::false_type {};

    template <>
    struct is_chunk_allocator<rdma_memory_pool>
      : std::false_type {};
}}
#endif

// ---------------------------------------------------------------------------
// pool_container, collect some routines for reuse with
// small, medium, large chunks
// ---------------------------------------------------------------------------
struct pool_container
{
    typedef std::function<rdma_memory_region_ptr(std::size_t)> regionAllocFunction;
#ifdef RDMA_HANDLER_HAVE_HPX
    typedef hpx::lcos::local::spinlock                mutex_type;
    typedef std::lock_guard<mutex_type>               scoped_lock;
    typedef std::unique_lock<mutex_type>              unique_lock;
    typedef hpx::lcos::local::condition_variable_any  condition_type;
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
            rdma_memory_region_ptr region = f(chunk_size_);
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
    void push(rdma_memory_region *region)
    {
        LOG_TRACE_MSG("Push block " << hexpointer(region->getAddress()) << hexlength(region->getLength()));
        // we must protect our mem buffer from thread contention
        unique_lock lock1(memBuffer_mutex_);
        //
#ifndef NDEBUG
        auto it = free_list_.begin();
        while (it != free_list_.end()) {
          if (*it == region) {
              LOG_ERROR_MSG("FATAL : Pushing an existing region back to the memory pool");
              std::terminate();
          }
          ++it;
        }
        free_list_.push_back(region);
#else
        free_list_.push(region);
#endif
        // decrement one reference
        region_use_count_--;
        lock1.unlock();
        // if anyone was waiting on the free list lock, then give it
        memBuffer_cond_.notify_one();
        LOG_DEBUG_MSG("memBuffer_cond_ notified one");
    }

    rdma_memory_region *pop()
    {
        unique_lock lock1(memBuffer_mutex_);
        // if we have not exceeded our max size, allocate a new block
        if (free_list_.empty() && block_list_.size()<max_chunks_) {
            // LOG_TRACE_MSG("Creating new small Block as free list is empty but max chunks " << max_small_chunks_ << " not reached");
            //  AllocateRegisteredBlock(length);
        }
        // make sure the list is not empty, wait on condition
        LOG_TRACE_MSG("Waiting to pop block");
        memBuffer_cond_.wait(lock1, [this] {
            return !free_list_.empty();
        });
        // get a block
#ifndef NDEBUG
        rdma_memory_region *region = free_list_.back();
        free_list_.pop_back();
#else
        rdma_memory_region *region = free_list_.top();
        free_list_.pop();
#endif
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
#ifndef NDEBUG
    std::deque<rdma_memory_region*>   free_list_;
#else
    std::stack<rdma_memory_region*>   free_list_;
#endif
    mutex_type                      memBuffer_mutex_;
    condition_type                  memBuffer_cond_;
    std::unordered_map<rdma_memory_region*, rdma_memory_region_ptr> block_list_;
};

// ---------------------------------------------------------------------------
// memory pool is a type of std:: compatible allocator
// ---------------------------------------------------------------------------
struct rdma_memory_pool : boost::noncopyable
{
    typedef std::size_t size_type;
    typedef char        value_type;

#ifdef RDMA_HANDLER_HAVE_HPX
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
    typedef rdma_protection_domainPtr protection_domain_type;
#endif

    // constructor
    rdma_memory_pool(protection_domain_type pd, std::size_t chunk_size, std::size_t init_chunks, std::size_t max_chunks);

    // destructor
    ~rdma_memory_pool();

    // set the protection domain, triggers pool deallocation and reallocation
    void setProtectionDomain(protection_domain_type pd);

    int   DeallocateList();
    int   DeallocateListBase();

    rdma_memory_region_ptr AllocateRegisteredBlock(std::size_t length);
    rdma_memory_region   *AllocateTemporaryBlock(std::size_t length);

    // -------------------------
    // User allocation interface
    // -------------------------
    // The rdma_memory_region* versions of allocate/deallocate
    // should be used in preference to the std:: compatible
    // versions using char* for efficiency

    bool canAllocateRegionUnsafe(size_t length);

    // allocate a region, if size=0 a small region is returned
    rdma_memory_region *allocateRegion(size_t size);

    // release a region back to the pool
    void deallocate(rdma_memory_region *region);

    // allocate a region, returning a memory block address
    char *allocate(size_t size);

    // deallocate a region using its memory address as handle
    // this involves a map lookup to find the region and is therefore
    // less efficient than releasing memory via the region pointer
    void deallocate(void *address, size_t size=0)
    {
        rdma_memory_region *region = pointer_map_[address];
        deallocate(region);
    }

    // find an rdma_memory_region* from the memory address it wraps
    rdma_memory_region *RegionFromAddress(char * const addr) {
        return pointer_map_[addr];
    }

    // utility functions
//    std::size_t small_chunk_size()  { return small_.chunk_size_; }
//    std::size_t medium_chunk_size() { return medium_.chunk_size_; }
//    std::size_t large_chunk_size()  { return large_.chunk_size_; }

    // used to map the internal memory address to the region that
    // holds the registration information
    std::unordered_map<const void *, rdma_memory_region*> pointer_map_;

    protection_domain_type    protection_domain_;
    bool                      isServer;

    // maintain 3 pools of pre-allocated regions
    pool_container small_;
    pool_container medium_;
    pool_container large_;
    std::atomic<int> temp_regions;

};

typedef std::shared_ptr<rdma_memory_pool> rdma_memory_poolPtr;

#endif
