#ifndef __system_allocator_hpp__
#define __system_allocator_hpp__

/*
  File
  ----
  system_allocator.hpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list and no system calls.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include <memory>

class system_alloc {
private:
  struct chunk_header;
  
  static chunk_header* first_header_;
  static chunk_header* last_header_;
  static const void* heap_end_;

  static chunk_header*
  find_header (chunk_header* start,
	       size_t size);

  static void
  split_header (chunk_header* ptr,
		size_t size);

  static bool
  merge_header (chunk_header* ptr);

  static void*
  allocate (size_t size);

public:
  static void
  initialize (void* begin,
	      const void* end);

  static const void*
  heap_begin ();

  static size_t
  heap_size ();

  static void*
  alloc (size_t) __attribute__((warn_unused_result));
  
  static void
  free (void*);
};

inline void*
operator new (size_t sz,
	      const system_alloc& pa)
{
  return pa.alloc (sz);
}

inline void*
operator new[] (size_t sz,
		const system_alloc& pa)
{
  return pa.alloc (sz);
}

template <class T>
inline void
destroy (T* p,
	 const system_alloc& la)
{
  if (p != 0) {
    p->~T ();
    la.free (p);
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
  
  static pointer
  address (reference r)
  {
    return &r;
  }

  static const_pointer
  address (const_reference r)
  {
    return &r;
  }
  
  system_allocator () { }

  template <class U>
  system_allocator (const system_allocator<U>&)
  { }

  static pointer
  allocate (size_type n,
	    std::allocator<void>::const_pointer = 0)
  {
    return static_cast<pointer> (system_alloc::alloc (n * sizeof (T)));
  }
  
  static void
  deallocate (pointer p,
	      size_type)
  {
    system_alloc::free (p);
  }
  
  static void
  construct (pointer p,
	     const T& val)
  {
    new (p) T (val);
  }

  static void
  destroy (pointer p)
  {
    p->~T ();
  }
  
  static size_type
  max_size ()
  {
    return static_cast<size_type> (-1) / sizeof (T);
  }
  
  template <class U>
  struct rebind {
    typedef system_allocator<U> other;
  };
};

#endif /* __system_allocator_hpp__ */
