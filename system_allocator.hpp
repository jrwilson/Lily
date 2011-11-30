#ifndef __system_allocator_h__
#define __system_allocator_hpp__

/*
  File
  ----
  system_allocator.hpp
  
  Description
  -----------
  The allocator for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include "list_allocator.hpp"
#include <memory>

typedef list_allocator system_allocator_type;
extern system_allocator_type sys_allocator;
struct system_allocator_tag { };

inline void*
operator new (size_t sz,
	      system_allocator_tag)
{
  return sys_allocator.alloc (sz);
}

inline void*
operator new[] (size_t sz,
		system_allocator_tag)
{
  return sys_allocator.alloc (sz);
}

template <class T>
void
destroy (T* p,
	 system_allocator_tag)
{
  if (p != 0) {
    p->~T ();
    sys_allocator.free (p);
  }
}

template <class T>
class system_allocator {
public:
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  
  typedef T* pointer;
  typedef const T* const_pointer;
  
  typedef T& reference;
  typedef const T& const_reference;
  
  pointer address (reference r) const { return &r; }
  const_pointer address (const_reference r) const { return &r; }
  
  system_allocator () { }
  system_allocator (const system_allocator&) { }
  ~system_allocator () { }
  template <class U>
  system_allocator (const system_allocator<U>&) { }
private:
  void operator= (const system_allocator&);
  
public:
  pointer allocate (size_type n,
		    std::allocator<void>::const_pointer = 0) {
    return static_cast<pointer> (sys_allocator.alloc (n * sizeof (T)));
  }
  
  void deallocate (pointer p,
		   size_type) {
    sys_allocator.free (p);
  }
  
  void construct (pointer p,
		  const T& val) { new (p) T (val); }
  void destroy (pointer p) { p->~T (); }
  
  size_type max_size () const { return static_cast<size_type> (-1) / sizeof (T);  }
  
  template <class U>
  struct rebind {
    typedef system_allocator<U> other;
  };
};

#endif /* __system_allocator_hpp__ */

