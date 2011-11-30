#ifndef __placement_allocator_h__
#define __placement_allocator_h__

/*
  File
  ----
  placement_allocator.h
  
  Description
  -----------
  A placement allocator.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include <memory>

class placement_alloc {
private:
  logical_address begin_;
  logical_address end_;
  logical_address limit_;
  
public:
  
  placement_alloc (const logical_address& begin,
		   const logical_address& limit);
  
  void*
  alloc (size_t size) __attribute__((warn_unused_result));
  
  inline logical_address
  begin (void) const
  {
    return begin_;
  }
  
  inline logical_address
  end (void) const
  {
    return end_;
  }
  
  inline logical_address
  limit (void) const
  {
    return limit_;
  }
  
};

inline void*
operator new (size_t sz,
	      placement_alloc& pa)
{
  return pa.alloc (sz);
}

inline void*
operator new[] (size_t sz,
		placement_alloc& pa)
{
  return pa.alloc (sz);
}

template <class T>
class placement_allocator {
private:
  placement_alloc& alloc_;

public:
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  
  typedef T* pointer;
  typedef const T* const_pointer;
  
  typedef T& reference;
  typedef const T& const_reference;
  
  pointer
  address (reference r) const
  {
    return &r;
  }

  const_pointer
  address (const_reference r) const
  {
    return &r;
  }
  
  placement_allocator (placement_alloc& a) :
    alloc_ (a)
  { }

  placement_allocator (const placement_allocator& other) :
    alloc_ (other.alloc_)
  { }

  ~placement_allocator ()
  { }

  template <class U>
  placement_allocator (const placement_allocator<U>& other) :
    alloc_ (other.alloc_)
  { }

private:
  void operator= (const placement_allocator&);
  
public:
  pointer
  allocate (size_type n,
	    std::allocator<void>::const_pointer = 0) {
    return static_cast<pointer> (alloc_.alloc (n * sizeof (T)));
  }
  
  void
  deallocate (pointer,
	      size_type) {
    // Do nothing.
  }
  
  void
  construct (pointer p,
	     const T& val) {
    new (p) T (val);
  }

  void
  destroy (pointer p) {
    p->~T ();
  }
  
  size_type
  max_size () const {
    return (alloc_.limit () - alloc_.begin ()) / sizeof (T);
  }
  
  template <class U>
  struct rebind {
    typedef placement_allocator<U> other;
  };
};

#endif /* __placement_allocator_h__ */
