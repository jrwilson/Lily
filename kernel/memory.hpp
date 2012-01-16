#ifndef __memory_hpp__
#define __memory_hpp__

#include "new.hpp"

template <typename T>
class allocator;

template <>
class allocator<void> {
  typedef void value_type;
  typedef void* pointer;
  typedef const void* const_pointer;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
};

template <typename T>
class allocator {
public:
  typedef T value_type;
  typedef T* pointer;
  typedef T& reference;
  typedef const T* const_pointer;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  allocator () { }
  allocator (const allocator&) { }
  template <typename U>
  allocator (const allocator<U>&) { }
  ~allocator () { }

  static inline pointer
  address (reference x)
  {
    return &x;
  }

  static inline const_pointer
  address (const_reference x)
  {
    return &x;
  }
  
  static inline pointer
  allocate (size_type n,
	    allocator<void>::const_pointer = 0)
  {
    return static_cast<pointer> (::operator new (n * sizeof (T)));
  }

  static inline void
  deallocator (pointer p,
	       size_type n)
  {
    ::operator delete (p);
  }

  static inline size_type
  max_size ()
  {
    return size_type (-1) / sizeof (T);
  }

  static void
  construct (pointer p,
	     const_reference val)
  {
    new (p) T (val);
  }

  static void
  destroy (pointer p)
  {
    p->~T ();
  }

  template <typename U>
  struct rebind {
    typedef allocator<U> other;
  };
};

#endif /* __memory_hpp__ */
