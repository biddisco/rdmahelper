#ifndef _RDMAREGISTEREDMEMORYPOOL_H
#define _RDMAREGISTEREDMEMORYPOOL_H

#include <ramdisk/include/services/common/RdmaProtectionDomain.h>
#include <ramdisk/include/services/common/RdmaMemoryRegion.h>
//
#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <mutex>
#include <iostream>
#include <iomanip>

using namespace bgcios;

struct pinned_allocator_malloc_free {

  //!< An unsigned integral type that can represent the size of the largest object to be allocated.
  typedef std::size_t size_type;
  //!< A signed integral type that can represent the difference of any two pointers.
  typedef std::ptrdiff_t difference_type;

  //@todo : protect access with mutex
  static RdmaProtectionDomainPtr getProtectionDomain() {
    return pinned_allocator_malloc_free::_protectionDomain;
  }

  static RdmaProtectionDomainPtr setProtectionDomain(RdmaProtectionDomainPtr pd) {
    pinned_allocator_malloc_free::_protectionDomain = pd;
  }

  static char * malloc BOOST_PREVENT_MACRO_SUBSTITUTION(const size_type bytes)
  {
    static bool first_time = true;
    char           *pblock = NULL;
    //
    if (first_time) {
      // allocate a block of 64kB using the BGVRNIC compatible routine
      if(!pinned_allocator_malloc_free::_region) {
        pinned_allocator_malloc_free::_region = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
      }
      RdmaProtectionDomainPtr pd = pinned_allocator_malloc_free::getProtectionDomain();
      pinned_allocator_malloc_free::_region->allocate(pd, bytes);
      pblock = (char*)_region->getAddress();
      first_time = false;
      LOG_DEBUG_MSG(__FUNCTION__ << " (" << std::dec << bytes << ") return " << std::hex << (uintptr_t)(pblock));
    }
    else {
      std::cout << "Buffer full, allocate squashed for " << std::dec << bytes << " bytes\n";
    }
    return pblock;
  }

  static void free BOOST_PREVENT_MACRO_SUBSTITUTION(char * const block)
  {
    // Block must have been previously returned from a call to UserAllocator::malloc.
    std::cout << __FUNCTION__ << "("<< std::hex << (uintptr_t)(block) << ")\n";
    delete [] block;
  }

  static RdmaProtectionDomainPtr _protectionDomain;
  static RdmaMemoryRegionPtr     _region;
  static std::mutex              _pd_mutex;
};
/*
class pinned_pool : public boost::object_pool<RdmaMemoryRegion, pinned_allocator_malloc_free>
{
  private:
    pinned_pool(const pinned_pool &);
    void operator=(const pinned_pool &);

  public:
    typedef boost::object_pool<RdmaMemoryRegion, pinned_allocator_malloc_free> superclass;
    typedef RdmaMemoryRegion element_type;
    typedef pinned_allocator_malloc_free user_allocator;
    typedef typename superclass::size_type size_type;
    typedef typename superclass::difference_type difference_type;
    //
    // we want each memory region to be 512 bytes with 32 allocated on start
    explicit pinned_pool(const size_type arg_next_size = 32, const size_type arg_max_size = 0) :
    boost::object_pool<RdmaMemoryRegion, pinned_allocator_malloc_free>(512, arg_next_size, arg_max_size) {}
    //
   ~pinned_pool() {};

    // can't overload 'construct' method by only changing return type, so use 'create' instead
    RdmaMemoryRegionPtr create() {
      RdmaMemoryRegion *region = superclass::construct();
      return RdmaMemoryRegionPtr(region);
    }
    void destroy(RdmaMemoryRegionPtr p) {
      superclass::destroy(p.get());
    }

};
*/

#endif
