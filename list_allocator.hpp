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

#include <memory>
#include "syscall.hpp"

// TODO:  Implement a more efficient allocator like Doug Lea's malloc (dlmalloc).

class syscall_sbrk {
public:
  void*
  operator () (size_t size)
  {
    return syscall::sbrk (size);
  }
};

template <typename Tag, class Sbrk = syscall_sbrk>
class list_alloc {
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

  struct data {
    Sbrk sbrk_;
    size_t page_size_;
    chunk_header* first_header_;
    chunk_header* last_header_;
  };

  static data data_;

  static inline size_t
  align_up (size_t value,
	    size_t radix)
  {
    return (value + radix - 1) & ~(radix - 1);
  }

  static chunk_header*
  find_header (chunk_header* start,
	       size_t size)
  {
    for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
    return start;
  }

  static void
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
    
    if (ptr == data_.last_header_) {
      data_.last_header_ = n;
    }
  }

  // Try to merge with the next chunk.
  static bool
  merge_header (chunk_header* ptr)
  {
    if (ptr->available && ptr->next != 0 && ptr->next->available && ptr->next == reinterpret_cast<chunk_header*>((reinterpret_cast<uint8_t*> (ptr) + sizeof (chunk_header) + ptr->size))) {
      ptr->size += sizeof (chunk_header) + ptr->next->size;
      if (ptr->next == data_.last_header_) {
	data_.last_header_ = ptr;
      }
      ptr->next = ptr->next->next;
      ptr->next->prev = ptr;
      return true;
    }
    else {
      return false;
    }
  }

public:
  static void
  initialize (size_t pagesize)
  {
    data_.page_size_ = pagesize;
    data_.first_header_ = new (data_.sbrk_ (data_.page_size_)) chunk_header (data_.page_size_ - sizeof (chunk_header));
    data_.last_header_ = data_.first_header_;
  }

  static void*
  alloc (size_t size)
  {
    if (size == 0) {
      return 0;
    }
    
    chunk_header* ptr = find_header (data_.first_header_, size);
    
    /* Acquire more memory. */
    if (ptr == 0) {
      size_t request_size = align_up (sizeof (chunk_header) + size, data_.page_size_);
      ptr = new (data_.sbrk_ (request_size)) chunk_header (request_size - sizeof (chunk_header));
      
      if (ptr > data_.last_header_) {
	// Append.
	data_.last_header_->next = ptr;
	// ptr->next already equals 0.
	ptr->prev = data_.last_header_;
	data_.last_header_ = ptr;
	if (merge_header (data_.last_header_->prev)) {
	  ptr = data_.last_header_;
	}
      }
      else if (ptr < data_.first_header_) {
	// Prepend.
	ptr->next = data_.first_header_;
	// ptr->prev already equals 0.
	data_.first_header_->prev = ptr;
	data_.first_header_ = ptr;
	merge_header (data_.first_header_);
      }
      else {
	// Insert.
	chunk_header* p;
	for (p = data_.first_header_; p < ptr; p = p->next) ;;
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
  
  static void
  free (void* p)
  {
    if (p == 0) {
      return;
    }
    
    chunk_header* ptr = static_cast<chunk_header*> (p);
    --ptr;
    
    if (ptr >= data_.first_header_ &&
	ptr <= data_.last_header_ &&
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

template <typename Tag, class Sbrk>
inline void*
operator new (size_t sz,
	      const list_alloc<Tag, Sbrk>& pa)
{
  return pa.alloc (sz);
}

template <typename Tag, class Sbrk>
inline void*
operator new[] (size_t sz,
		const list_alloc<Tag, Sbrk>& pa)
{
  return pa.alloc (sz);
}

template <class T, typename Tag, class Sbrk>
inline void
destroy (T* p,
	 const list_alloc<Tag, Sbrk>& la)
{
  if (p != 0) {
    p->~T ();
    la.free (p);
  }
}

template <class T, typename Tag, class Sbrk = syscall_sbrk>
class list_allocator {
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
  
  list_allocator () { }

  list_allocator (const list_allocator&)
  { }

  ~list_allocator ()
  { }

  template <class U>
  list_allocator (const list_allocator<U, Tag, Sbrk>&)
  { }

  static pointer
  allocate (size_type n,
	    std::allocator<void>::const_pointer = 0)
  {
    return static_cast<pointer> (list_alloc<Tag, Sbrk>::alloc (n * sizeof (T)));
  }
  
  static void
  deallocate (pointer p,
	      size_type)
  {
    list_alloc<Tag, Sbrk>::free (p);
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
    typedef list_allocator<U, Tag, Sbrk> other;
  };
};

#endif /* __list_allocator_hpp__ */
