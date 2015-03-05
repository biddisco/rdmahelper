#ifndef _RDMAREGISTEREDMEMORYPOOL_H
#define _RDMAREGISTEREDMEMORYPOOL_H

#include <RdmaProtectionDomain.h>
#include <RdmaMemoryRegion.h>
//
#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <mutex>
#include <set>
#include <iostream>
#include <iomanip>

using namespace bgcios;

struct fixed_allocator_new_delete {
  //!< An unsigned integral type that can represent the size of the largest object to be allocated.
  typedef std::size_t size_type;
  //!< A signed integral type that can represent the difference of any two pointers.
  typedef std::ptrdiff_t difference_type;

  static char * malloc BOOST_PREVENT_MACRO_SUBSTITUTION(const size_type bytes)
  {
    //! Attempts to allocate n bytes from the system. Returns 0 if out-of-memory
    static bool first_time=true;
    char* pblock=NULL;
    if (first_time)
    {
      pblock = new (std::nothrow) char[bytes];
      first_time=false;
      std::cout << __FUNCTION__ << "(" << std::dec << bytes << ") return " << std::hex << (uintptr_t)(pblock) << " ";
    }
    else {
      std::cout << "Buffer full, allocate squashed asked for " << std::dec << bytes << "\n";
    }
    return pblock;
  }

  static void free BOOST_PREVENT_MACRO_SUBSTITUTION(char * const block)
  { //! Attempts to de-allocate block.
    //! \pre Block must have been previously returned from a call to UserAllocator::malloc.
    std::cout << __FUNCTION__ << "("<< std::hex << (uintptr_t)(block) << ")\n";
    delete [] block;
  }
};

//struct pinned_allocator_malloc_free {
//
//  //!< An unsigned integral type that can represent the size of the largest object to be allocated.
//  typedef std::size_t size_type;
//  //!< A signed integral type that can represent the difference of any two pointers.
//  typedef std::ptrdiff_t difference_type;
//
//  //@todo : protect access with mutex
//  static RdmaProtectionDomainPtr getProtectionDomain() {
//    return pinned_allocator_malloc_free::_protectionDomain;
//  }
//
//  static RdmaProtectionDomainPtr setProtectionDomain(RdmaProtectionDomainPtr pd) {
//    pinned_allocator_malloc_free::_protectionDomain = pd;
//  }
//
//  static char * malloc BOOST_PREVENT_MACRO_SUBSTITUTION(const size_type bytes)
//  {
//    static bool first_time = true;
//    char           *pblock = NULL;
//    //
//    if (first_time) {
//      // allocate a block of 64kB using the BGVRNIC compatible routine
//      if(!pinned_allocator_malloc_free::_region) {
//        pinned_allocator_malloc_free::_region = RdmaMemoryRegionPtr(new RdmaMemoryRegion());
//      }
//      RdmaProtectionDomainPtr pd = pinned_allocator_malloc_free::getProtectionDomain();
//      pinned_allocator_malloc_free::_region->allocate(pd, bytes);
//      pblock = (char*)_region->getAddress();
//      first_time = false;
//      LOG_DEBUG_MSG(__FUNCTION__ << " (" << std::dec << bytes << ") return " << std::hex << (uintptr_t)(pblock));
//    }
//    else {
//      std::cout << "Buffer full, allocate squashed for " << std::dec << bytes << " bytes\n";
//    }
//    return pblock;
//  }
//
//  static void free BOOST_PREVENT_MACRO_SUBSTITUTION(char * const block)
//  {
//    // Block must have been previously returned from a call to UserAllocator::malloc.
//    std::cout << __FUNCTION__ << "("<< std::hex << (uintptr_t)(block) << ")\n";
//    delete [] block;
//  }
//
//  static RdmaProtectionDomainPtr _protectionDomain;
//  static RdmaMemoryRegionPtr     _region;
//  static std::mutex              _pd_mutex;
//};

class RdmaRegisteredMemoryPool : public boost::object_pool<RdmaMemoryRegion, fixed_allocator_new_delete>
{
  private:
  RdmaRegisteredMemoryPool(const RdmaRegisteredMemoryPool &);
    void operator=(const RdmaRegisteredMemoryPool &);

  public:
    typedef boost::object_pool<RdmaMemoryRegion, fixed_allocator_new_delete> superclass;
    typedef RdmaMemoryRegion                     element_type;
    typedef fixed_allocator_new_delete           user_allocator;
    typedef typename superclass::size_type       size_type;
    typedef typename superclass::difference_type difference_type;
    //
    // we want 32 allocated on start by default
    explicit RdmaRegisteredMemoryPool(RdmaProtectionDomainPtr pd, const size_type arg_next_size = 32, const size_type arg_max_size = 0)
      : _protectionDomain(pd), superclass(arg_next_size) {}

    //
   ~RdmaRegisteredMemoryPool() {
     for (auto b : this->_registered_Blocks) {
       b->release();
     }
   }

  void free(element_type *region)
  {
    std::cout << "Freeing region from pool with address " << (uintptr_t)region << std::endl;

    memset(region->getAddress(),0,512);

    std::cout << "wiped it " << (uintptr_t)region << " key " << region->getLocalKey() << " at address " << region->getAddress() << std::endl;
    superclass::free(region);

    std::cout << "freed  region " << (uintptr_t)region << " key " << region->getLocalKey() << " at address " << region->getAddress() << std::endl;

    memset(region->getAddress(),0,512);

    std::cout << "freed  region (and wiped it)" << (uintptr_t)region << " key " << region->getLocalKey() << " at address " << region->getAddress() << std::endl;

  }

    // can't overload 'construct' method by only changing return type (RdmaMemoryRegion) ,
    // so use 'create' instead to return a smart pointer
    element_type *create() {
      element_type *region = superclass::construct();
      std::cout << "Created region from pool with address " << (uintptr_t)region << std::endl;
      if (this->_registered_Blocks.find(region)==this->_registered_Blocks.end()) {
        region->allocate(this->_protectionDomain, 512);
        std::cout << "allocated 512 bytes in region space for region " << (uintptr_t)region << " key " << region->getLocalKey() << " at address " << region->getAddress() << std::endl;
        this->_registered_Blocks.insert(region);
        region->setMessageLength(512);
      }
      return region;
    }


    RdmaProtectionDomainPtr _protectionDomain;
    std::set<element_type*> _registered_Blocks;
};

typedef std::shared_ptr<RdmaRegisteredMemoryPool> RdmaRegisteredMemoryPoolPtr;

#endif
