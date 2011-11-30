#ifndef __list_allocator_hpp__
#define __list_allocator_hpp__

/*
  File
  ----
  list_allocator.hpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include <memory>

class list_alloc {
private:
  struct chunk_header;
  
  size_t page_size_;
  chunk_header* first_header_;
  chunk_header* last_header_;

  chunk_header*
  find_header (chunk_header* start,
	       size_t size);

  void
  split_header (chunk_header* ptr,
		size_t size);
  
public:
  list_alloc ();

  void*
  alloc (size_t) __attribute__((warn_unused_result));
  
  void
  free (void*);
};

inline void*
operator new (size_t sz,
	      list_alloc& pa)
{
  return pa.alloc (sz);
}

inline void*
operator new[] (size_t sz,
		list_alloc& pa)
{
  return pa.alloc (sz);
}

template <class T>
inline void
destroy (T* p,
	 list_alloc& la)
{
  if (p != 0) {
    p->~T ();
    la.free (p);
  }
}

template <class T>
class list_allocator {
private:
  list_alloc& alloc_;

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
  
  list_allocator (list_alloc& a) :
    alloc_ (a)
  { }

  list_allocator (const list_allocator& other) :
    alloc_ (other.alloc_)
  { }

  ~list_allocator ()
  { }

  template <class U>
  list_allocator (const list_allocator<U>& other) :
    alloc_ (other.get_alloc ())
  { }

private:
  void operator= (const list_allocator&);
  
public:
  pointer
  allocate (size_type n,
	    std::allocator<void>::const_pointer = 0) {
    return static_cast<pointer> (alloc_.alloc (n * sizeof (T)));
  }
  
  void
  deallocate (pointer p,
	      size_type) {
    alloc_.free (p);
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
    return static_cast<size_type> (-1) / sizeof (T);
  }
  
  template <class U>
  struct rebind {
    typedef list_allocator<U> other;
  };

  list_alloc&
  get_alloc () const
  {
    return alloc_;
  }
};

#endif /* __list_allocator_hpp__ */
