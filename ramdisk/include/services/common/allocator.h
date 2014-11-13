#ifndef __allocator_hpp_
#define __allocator_hpp_

//
#include <memory>
//
#include "memory_pool.hpp"
//
using namespace std;
using boost::format;
using boost::io::group;

//----------------------------------------------------------------------------
// Allocator which takes a block from a pre-allocated pool.
//
// Used by VirtualNVP storage which must pin all memory blocks
// used for transfers. 
//----------------------------------------------------------------------------
template <typename T>
class allocator: public std::allocator<T>
{
public:
  typedef T              value_type;
  typedef T*             pointer;
  typedef const T*       const_pointer;
  typedef T&             reference;
  typedef const T&       const_reference;
  typedef std::size_t    size_type;
  typedef std::ptrdiff_t difference_type;

  allocator() throw() : memory_pool_(0), std::allocator<T>() { HPX_ASSERT(false); };
  allocator(memory_pool & mp) throw() : memory_pool_(&mp) {};
  allocator(allocator const & other) throw() : memory_pool_(other.memory_pool_), std::allocator<T>(other) {};

  ~allocator() throw() {}

  // we want to make sure anything else uses the std allocator
  template <class U> 
  struct rebind { typedef std::allocator<U> other; };

  pointer address (reference value) const {
    return &value;
  }
  const_pointer address (const_reference value) const {
    return &value;
  }

  pointer allocate(size_type n, const void *hint=0)
  {
    pointer *buffer = memory_pool_->allocate(n);
    //
    char const*  msg = "Allocating %1% %2% from pool ";
    std::cout << (boost::format(msg) % n % group(setfill('0'), hex, setw(12), (void*)buffer)) << std::endl;
    //
    return buffer;
  }

  void deallocate(pointer p, size_type n)
  {
    char const*  msg = "Allocator returning %1% to pool ";
    std::cout << (boost::format(msg) % group(setfill('0'), hex, setw(12), (void*)p)) << std::endl;
    //
    boost::shared_ptr<abstract> storage = abstract::GetInstance();
    memory_poool_->deallocate(p);
  }

  // initialize elements of allocated storage p with value value
  void construct (pointer p, const T& value) {
    char const*  msg = "Allocator constructing %1% objects";
    std::cout << (boost::format(msg) % value) << std::endl << std::flush;
    new ((void*)p)T(value);
  }

  void destroy (pointer p) {
    char const*  msg = "Allocator deleting %1% objects (address %2%) ";
    std::cout << (boost::format(msg) % p) << std::endl << std::flush;
    p->~T();
  }



  memory_pool *memory_pool_;
};

#endif
