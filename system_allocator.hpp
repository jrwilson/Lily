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
#include "automaton_interface.hpp"

class system_alloc {
private:
  enum status {
    NORMAL,
    BOOTING,
  };

  struct chunk_header;

  status status_;
  uint8_t* boot_begin_;
  uint8_t* boot_end_;

  automaton_interface* automaton_;
  chunk_header* first_header_;
  chunk_header* last_header_;

  chunk_header*
  find_header (chunk_header* start,
	       size_t size) const;

  void
  split_header (chunk_header* ptr,
		size_t size);

  bool
  merge_header (chunk_header* ptr);

  void*
  allocate (size_t size);

public:
  void
  boot (const void* boot_begin,
	const void* boot_end);

  void
  normal (automaton_interface* a);

  void*
  alloc (size_t) __attribute__((warn_unused_result));
  
  void
  free (void*);
};

inline void*
operator new (size_t sz,
	      system_alloc& pa)
{
  return pa.alloc (sz);
}

inline void*
operator new[] (size_t sz,
		system_alloc& pa)
{
  return pa.alloc (sz);
}

template <class T>
inline void
destroy (T* p,
	 system_alloc& la)
{
  if (p != 0) {
    p->~T ();
    la.free (p);
  }
}

template <class T>
class system_allocator {
private:
  system_alloc& alloc_;

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
  
  system_allocator (system_alloc& a) :
    alloc_ (a)
  { }

  system_allocator (const system_allocator& other) :
    alloc_ (other.alloc_)
  { }

  ~system_allocator ()
  { }

  template <class U>
  system_allocator (const system_allocator<U>& other) :
    alloc_ (other.get_alloc ())
  { }

private:
  void operator= (const system_allocator&);
  
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
    typedef system_allocator<U> other;
  };

  system_alloc&
  get_alloc () const
  {
    return alloc_;
  }
};

#endif /* __system_allocator_hpp__ */
