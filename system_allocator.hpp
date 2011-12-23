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

#include <memory>
#include <stdint.h>
#include "vm_def.hpp"
#include "kassert.hpp"

class system_alloc {
private:
  static const uint32_t MAGIC = 0x7ACEDEAD;

  struct chunk_header {
    uint32_t available : 1;
    uint32_t magic : 31;
    size_t size; /* Does not include header. */
    chunk_header* prev;
    chunk_header* next;
    
    chunk_header (size_t sz) :
      available (1),
      magic (MAGIC),
      size (sz),
      prev (0),
      next (0)
    { }
  };
  
  static chunk_header* first_header_;
  static chunk_header* last_header_;
  static logical_address_t heap_end_;
  static bool backing_;

  static inline chunk_header*
  find_header (chunk_header* start,
	       size_t size)
  {
    for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
    return start;
  }

  static inline void
  split_header (chunk_header* ptr,
		size_t size)
  {
    /* Split the block. */
    chunk_header* n = new (reinterpret_cast<uint8_t*> (ptr) + sizeof (chunk_header) + size) chunk_header (ptr->size - size - sizeof (chunk_header));
    n->prev = ptr;
    n->next = ptr->next;
    
    ptr->size = size;
    ptr->next = n;
    
    if (n->next != 0) {
      n->next->prev = n;
    }
    
    if (ptr == last_header_) {
      last_header_ = n;
    }
  }

  static inline bool
  merge_header (chunk_header* ptr)
  {
    if (ptr->available && ptr->next != 0 && ptr->next->available && ptr->next == reinterpret_cast<chunk_header*>((reinterpret_cast<uint8_t*> (ptr) + sizeof (chunk_header) + ptr->size))) {
      ptr->size += sizeof (chunk_header) + ptr->next->size;
      if (ptr->next == last_header_) {
	last_header_ = ptr;
      }
      ptr->next = ptr->next->next;
      ptr->next->prev = ptr;
      return true;
    }
    else {
      return false;
    }
  }

  static void*
  allocate (size_t size);

public:
  static void
  initialize (logical_address_t begin,
	      logical_address_t end)
  {
    // We use a page as the initial size of the heap.
    const size_t request_size = PAGE_SIZE;
    kassert (begin < end);
    kassert (end - begin >= request_size);
    
    first_header_ = new (reinterpret_cast<void*> (begin)) chunk_header (request_size - sizeof (chunk_header));
    last_header_ = first_header_;
    heap_end_ = end;
  }

  static void
  engage_vm (logical_address_t end)
  {
    heap_end_ = end;
    backing_ = true;
  }
  
  static logical_address_t
  heap_begin ()
  {
    return reinterpret_cast<logical_address_t> (first_header_);
  }

  static logical_address_t
  heap_end ()
  {
    return reinterpret_cast<logical_address_t> (last_header_ + 1) + last_header_->size;
  }

  static inline void*
  alloc (size_t size)
  {
    if (size == 0) {
      return 0;
    }
    
    chunk_header* ptr = find_header (first_header_, size);
    
    /* Acquire more memory. */
    if (ptr == 0) {
      const size_t request_size = align_up (size + sizeof (chunk_header), PAGE_SIZE);
      ptr = new (allocate (request_size)) chunk_header (request_size - sizeof (chunk_header));
      
      if (ptr > last_header_) {
	// Append.
	last_header_->next = ptr;
	// ptr->next already equals 0.
	ptr->prev = last_header_;
	last_header_ = ptr;
	if (merge_header (last_header_->prev)) {
	  ptr = last_header_;
	}
      }
      else if (ptr < first_header_) {
	// Prepend.
	ptr->next = first_header_;
	// ptr->prev already equals 0.
	first_header_->prev = ptr;
	first_header_ = ptr;
	merge_header (first_header_);
      }
      else {
	// Insert.
	chunk_header* p;
	for (p = first_header_; p < ptr; p = p->next) ;;
	ptr->next = p;
	ptr->prev = p->prev;
	p->prev = ptr;
	ptr->prev->next = ptr;
	merge_header (ptr);
	p = ptr->prev;
	if (merge_header (p)) {
	  ptr = p;
	}
      }
    }
    
    /* Found something we can use. */
    if ((ptr->size - size) > sizeof (chunk_header)) {
      split_header (ptr, size);
    }
    /* Mark as used and return. */
    ptr->available = 0;
    return (ptr + 1);
  }
  
  static inline void
  free (void* p)
  {
    if (p == 0) {
      return;
    }
    
    chunk_header* ptr = static_cast<chunk_header*> (p);
    --ptr;
    
    if (ptr >= first_header_ &&
	ptr <= last_header_ &&
	ptr->magic == MAGIC &&
	ptr->available == 0) {
      
      ptr->available = 1;
      
      // Merge with previous.
      merge_header (ptr);
      
      // Merge with previous.
      if (ptr->prev != 0) {
	merge_header (ptr->prev);
      }
    }
  }

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
