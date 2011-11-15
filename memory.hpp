#ifndef __memory_hpp__
#define __memory_hpp__

/*
  File
  ----
  memory.hpp
  
  Description
  -----------
  Functions and classes for memory allocation.

  Authors:
  Justin R. Wilson
*/

#include "new.hpp"
#include "iterator.hpp"

/* From Chapter 19 of the C++ Programming Language. */

template <class T> class allocator;

template <>
class allocator<void> {
public:
  typedef void* pointer;
  typedef const void* const_pointer;
  typedef void value_type;
  template <class U>
  struct rebind {
    typedef allocator<U> other;
  };
};

template <class T>
class allocator {
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

  allocator () { }
  allocator (const allocator&) { }
  ~allocator () { }
  template <class U>
  allocator (const allocator<U>&) { }
private:
  void operator= (const allocator&);

public:
  pointer allocate (size_type n,
		    allocator<void>::const_pointer = 0) {
    if (n == 1) {
      return static_cast<pointer> (operator new (sizeof (T)));
    }
    else {
      return static_cast<pointer> (operator new[] (n * sizeof (T)));
    }
  }

  void deallocate (pointer p,
		   size_type n) {
    if (n == 1) {
      return operator delete (p);
    }
    else {
      return operator delete[] (p);
    }
  }

  void construct (pointer p,
		  const T& val) { new (p) T (val); }
  void destroy (pointer p) { p->~T (); }

  size_type max_size () const { return static_cast<size_type> (-1) / sizeof (T);  }

  template <class U>
  struct rebind {
    typedef allocator<U> other;
  };
  
};

template <class T>
bool operator== (const allocator<T>&,
		 const allocator<T>&) { return true; }

template <class T>
bool operator!= (const allocator<T>&,
		 const allocator<T>&) { return false; }

#endif /* __memory_hpp_ */
