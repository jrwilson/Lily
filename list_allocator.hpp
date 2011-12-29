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
#include "kassert.hpp"

class syscall_syscall {
public:
  void*
  sbrk (size_t size) const
  {
    return syscall::sbrk (size);
  }

  size_t
  getpagesize () const
  {
    return syscall::getpagesize ();
  }
};


static const size_t BIN_COUNT = 128;
extern const size_t bin_size[63];

template <typename Tag, class Syscall = syscall_syscall>
class list_alloc {
private:

  static const size_t ALLOC_ALIGN = 8;
  static const size_t ALLOC_MIN = 8;

  // TODO:  We assume a 32-bit architecture.

  class header;

  class footer {
  private:
    size_t size_;

  public:
    footer (size_t size) :
      size_ (size)
    { }

    inline size_t
    size () const
    {
      return size_;
    }

    inline header*
    get_header ()
    {
      return reinterpret_cast<header*> (this) - 1 - size_ / sizeof (footer);
    }

    inline header*
    get_header () const
    {
      return reinterpret_cast<header*> (this) - 1 - size_ / sizeof (footer);
    }

    inline footer*
    prev ()
    {
      return this - 2 - size_ / sizeof (footer);
    }

    inline const footer*
    prev () const
    {
      return this - 2 - size_ / sizeof (footer);
    }
  };

  class header {
  private:
    size_t available_ : 1;
    size_t size_ : 31;

  public:
    header (size_t size) :
      available_ (true),
      size_ (size)
    {
      new (get_footer ()) footer (size_);
    }

    inline bool
    available () const
    {
      return available_;
    }

    inline void
    available (bool a)
    {
      available_ = a;
    }

    inline size_t
    size () const
    {
      return size_;
    }

    inline footer*
    get_footer ()
    {
      return reinterpret_cast<footer*> (this) + 1 + size_ / sizeof (header);
    }

    inline const footer*
    get_footer () const
    {
      return this + 1 + size_ / sizeof (header);
    }

    inline header*
    next ()
    {
      return this + 2 + size_ / sizeof (header);
    }

    inline const header*
    next () const
    {
      return this + 2 + size_ / sizeof (header);
    }

    // Try to merge with the next chunk.
    inline void
    merge (const header* n)
    {
      size_ += n->size () + 2 * sizeof (header);
      new (get_footer ()) footer (size_);
    }

    inline header*
    split (size_t size)
    {
      if ((size_ - size) < 2 * sizeof (header) + ALLOC_MIN) {
	return 0;
      }
      else {
	header* n = this + 2 + size / sizeof (header);
	new (n) header (size_ - size - 2 * sizeof (header));
	new (this) header (size);
	return n;
      }
    }

    inline header*&
    free_next ()
    {
      return reinterpret_cast<header*&> (*(this + 1));
    }

    inline header*&
    free_prev ()
    {
      return reinterpret_cast<header*&> (*(this + 2));
    }

    inline void*
    data ()
    {
      return this + 1;
    }
  };

  struct data {
    Syscall syscall_;
    size_t page_size_;
    header* first_header_;
    header* last_header_;
    header* bin_[BIN_COUNT];
  };

  static data data_;

  static inline size_t
  align_up (size_t value,
	    size_t radix)
  {
    return (value + radix - 1) & ~(radix - 1);
  }

  static inline size_t
  size_to_bin (size_t size)
  {
    if (size <= 512) {
      return size / 8;
    }
    else {
      return 65 + std::lower_bound (bin_size, bin_size + 63, size) - bin_size;
    }
  }

  static inline void
  insert (header* h)
  {
    header* prev;
    header* next;
    const size_t idx = size_to_bin (h->size ());
    for (prev = 0, next = data_.bin_[idx];
    	 next != 0 && (next->size () < h->size () || next < h);
    	 prev = next, next = next->free_next ()) ;;
    
    if (next != 0) {
      next->free_prev () = h;
    }
    
    h->free_next () = next;
    h->free_prev () = prev;

    if (prev != 0) {
      prev->free_next () = h;
    }
    else {
      data_.bin_[idx] = h;
    }
  }

  static inline void
  remove (header* h)
  {
    header* prev = h->free_prev ();
    header* next = h->free_next ();

    if (prev != 0) {
      prev->free_next () = next;
    }
    else {
      data_.bin_[size_to_bin (h->size ())] = next;
    }

    if (next != 0) {
      next->free_prev () = prev;
    }
  }

public:
  static void
  initialize ()
  {
    data_.page_size_ = data_.syscall_.getpagesize ();
    data_.first_header_ = new (data_.syscall_.sbrk (data_.page_size_)) header (data_.page_size_ - 2 * sizeof (header));
    data_.last_header_ = data_.first_header_;
    for (size_t idx = 0; idx != BIN_COUNT; ++idx) {
      data_.bin_[idx] = 0;
    }
  }

  static inline void*
  alloc (size_t size)
  {
    if (size == 0) {
      return 0;
    }

    // Increase the size to the minimum and align.
    size = align_up (std::max (size, ALLOC_MIN), ALLOC_ALIGN);

    // Try to allocate a chunk from the list of free chunks.
    for (size_t idx = size_to_bin (size); idx != BIN_COUNT; ++idx) {
      for (header* ptr = data_.bin_[idx]; ptr != 0; ptr = ptr->free_next ()) {
      	if (ptr->size () >= size) {
	  // Remove from the free list.
	  remove (ptr);
	  // Try to split.
	  header* s = ptr->split (size);
	  if (s != 0) {
	    insert (s);
	  }
	  // Mark as used and return.
	  ptr->available (false);
	  return ptr->data ();
	}
      }
    }

    // Allocating a chunk from the free list failed.
    // Use the last chunk.
    if (!data_.last_header_->available ()) {
      // The last chunk is not available.
      // Create one.
      size_t request_size = align_up (size + 2 * sizeof (header), data_.page_size_);
      // TODO:  Check the return value.
      data_.last_header_ = new (data_.syscall_.sbrk (request_size)) header (request_size - 2 * sizeof (header));
    }
    else if (data_.last_header_->size () < size) {
      // The last chunk is available but too small.
      // Resize the last chunk.
      size_t request_size = align_up (size - data_.last_header_->size (), data_.page_size_);
	// TODO:  Check the return value.
      data_.syscall_.sbrk (request_size);
      new (data_.last_header_) header (data_.last_header_->size () + request_size);
    }

    // The last chunk is now of sufficient size.  Try to split.
    header* ptr = data_.last_header_;
    header* s = ptr->split (size);
    if (s != 0) {
      data_.last_header_ = s;
    }

    // Mark as used and return.
    ptr->available (false);

    return ptr->data ();
  }
  
  static inline void
  free (void* p)
  {
    if (p == 0) {
      return;
    }

    header* h = static_cast<header*> (p);
    --h;

    if (h >= data_.first_header_ &&
    	h <= data_.last_header_ &&
    	!h->available ()) {

      footer* f = h->get_footer ();

      if (f >= data_.first_header_->get_footer () &&
	  f <= data_.last_header_->get_footer () &&
	  h->size () == f->size ()) {
	// We are satisfied that the chunk is correct.
	// We could be more paranoid and check the previous/next chunk.

	// Chunk is now available.
	h->available (true);

	header* next = h->next ();
	if (h != data_.last_header_ && next->available ()) {
	  if (next != data_.last_header_) {
	    // Remove from the free list.
	    remove (next);
	  }
	  else {
	    // Become the last.
	    data_.last_header_ = h;
	  }
	  // Merge with next.
	  h->merge (next);
	}

	header* prev = h->get_footer ()->prev ()->get_header ();
	if (h == data_.first_header_ || !prev->available ()) {
	  //Insert.
	  insert (h);
	}
	else {
	  // Remove from the free list.
	  remove (prev);
	  // Merge.
	  prev->merge (h);
	  if (h != data_.last_header_) {
	    // Place the merged chunk on the free list.
	    insert (prev);
	  }
	  else {
	    // Become the last.
	    data_.last_header_ = prev;
	  }
	}
      }
    }
  }
  
};

template <typename T1, typename T2>
typename list_alloc<T1, T2>::data list_alloc<T1, T2>::data_;

template <typename Tag, class Syscall>
inline void*
operator new (size_t sz,
	      const list_alloc<Tag, Syscall>& pa)
{
  return pa.alloc (sz);
}

template <typename Tag, class Syscall>
inline void*
operator new[] (size_t sz,
		const list_alloc<Tag, Syscall>& pa)
{
  return pa.alloc (sz);
}

template <class T, typename Tag, class Syscall>
inline void
destroy (T* p,
	 const list_alloc<Tag, Syscall>& la)
{
  if (p != 0) {
    p->~T ();
    la.free (p);
  }
}

template <class T, typename Tag, class Syscall = syscall_syscall>
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
  list_allocator (const list_allocator<U, Tag, Syscall>&)
  { }

  static pointer
  allocate (size_type n,
	    std::allocator<void>::const_pointer = 0)
  {
    return static_cast<pointer> (list_alloc<Tag, Syscall>::alloc (n * sizeof (T)));
  }
  
  static void
  deallocate (pointer p,
	      size_type)
  {
    list_alloc<Tag, Syscall>::free (p);
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
    typedef list_allocator<U, Tag, Syscall> other;
  };
};

#endif /* __list_allocator_hpp__ */
