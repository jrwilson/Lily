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
#include <algorithm>

// TODO:  Refactor for data hiding and namespace pollution.
// TODO:  We assume a 32-bit architecture.

static const size_t BIN_COUNT = 128;
extern const size_t bin_size[63];

static const size_t ALLOC_ALIGN = 8;
static const size_t ALLOC_MIN = 8;

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
    return reinterpret_cast<header*> (this - 1 - size_ / sizeof (footer));
  }
  
  inline const header*
  get_header () const
  {
    return reinterpret_cast<const header*> (this - 1 - size_ / sizeof (footer));
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
    return reinterpret_cast<footer*> (this + 1 + size_ / sizeof (header));
  }
  
  inline const footer*
  get_footer () const
  {
    return reinterpret_cast<const footer*> (this + 1 + size_ / sizeof (header));
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

struct list_alloc_state {
  size_t page_size_;
  header* first_header_;
  header* last_header_;
  header* bin_[BIN_COUNT];
};

class list_alloc_syscall {
public:
  static void*
  sbrk (size_t size)
  {
    return syscall::sbrk (size);
  }

  static size_t
  getpagesize ()
  {
    return syscall::getpagesize ();
  }
};

template <class Syscall>
class list_alloc : private Syscall {
private:
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
    for (prev = 0, next = Syscall::state ().bin_[idx];
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
      Syscall::state ().bin_[idx] = h;
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
      Syscall::state ().bin_[size_to_bin (h->size ())] = next;
    }

    if (next != 0) {
      next->free_prev () = prev;
    }
  }

  // static void
  // check ()
  // {
  //   // Scan the free list.
  //   size_t free = 0;
  //   for (size_t idx = 0; idx < BIN_COUNT; ++idx) {
  //     header* h = Syscall::state ().bin_[idx];
  //     while (h != 0) {
  // 	kassert (Syscall::state ().first_header_ <= h);
  // 	kassert (h < Syscall::state ().last_header_);
  // 	kassert (h->available ());
  // 	++free;
  // 	kassert (h->size () >= ALLOC_MIN);
  // 	footer* f = h->get_footer ();
  // 	kassert (h->size () == f->size ());
  // 	kassert (f->get_header () == h);

  // 	header* t;
  // 	t = h->free_next ();
  // 	kassert (t == 0 || (Syscall::state ().first_header_ <= t && t < Syscall::state ().last_header_));
  // 	t = h->free_prev ();
  // 	kassert (t == 0 || (Syscall::state ().first_header_ <= t && t < Syscall::state ().last_header_));

  // 	h = h->free_next ();
  //     }
  //   }

  //   // Scan all of the chunks.
  //   size_t chunks = 0;
  //   size_t available = 0;
  //   for (header* ptr = Syscall::state ().first_header_; ptr <= Syscall::state ().last_header_; ptr = ptr->next ()) {
  //     ++chunks;
  //     if (ptr->available ()) {
  // 	++available;
  //     }
  //   }
  //   kassert (free <= available);
  //   kassert (available <= chunks);
  //   kout << "\t" << free << "/" << available << "/" << chunks << endl;
  // }

public:
  static void
  initialize ()
  {
    Syscall::state ().page_size_ = Syscall::getpagesize ();
    Syscall::state ().first_header_ = new (Syscall::sbrk (Syscall::state ().page_size_)) header (Syscall::state ().page_size_ - 2 * sizeof (header));
    Syscall::state ().last_header_ = Syscall::state ().first_header_;
    for (size_t idx = 0; idx != BIN_COUNT; ++idx) {
      Syscall::state ().bin_[idx] = 0;
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
      for (header* ptr = Syscall::state ().bin_[idx]; ptr != 0; ptr = ptr->free_next ()) {
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
    if (!Syscall::state ().last_header_->available ()) {
      // The last chunk is not available.
      // Create one.
      size_t request_size = align_up (size + 2 * sizeof (header), Syscall::state ().page_size_);
      // TODO:  Check the return value.
      Syscall::state ().last_header_ = new (Syscall::sbrk (request_size)) header (request_size - 2 * sizeof (header));
    }
    else if (Syscall::state ().last_header_->size () < size) {
      // The last chunk is available but too small.
      // Resize the last chunk.
      size_t request_size = align_up (size - Syscall::state ().last_header_->size (), Syscall::state ().page_size_);
	// TODO:  Check the return value.
      Syscall::sbrk (request_size);
      new (Syscall::state ().last_header_) header (Syscall::state ().last_header_->size () + request_size);
    }

    // The last chunk is now of sufficient size.  Try to split.
    header* ptr = Syscall::state ().last_header_;
    header* s = ptr->split (size);
    if (s != 0) {
      Syscall::state ().last_header_ = s;
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

    if (h >= Syscall::state ().first_header_ &&
    	h <= Syscall::state ().last_header_ &&
    	!h->available ()) {

      footer* f = h->get_footer ();

      if (f >= Syscall::state ().first_header_->get_footer () &&
	  f <= Syscall::state ().last_header_->get_footer () &&
	  h->size () == f->size ()) {
	// We are satisfied that the chunk is correct.
	// We could be more paranoid and check the previous/next chunk.

	// Chunk is now available.
	h->available (true);

	if (h != Syscall::state ().last_header_) {
	  header* next = h->next ();
	  if (next->available ()) {
	    if (next != Syscall::state ().last_header_) {
	      // Remove from the free list.
	      remove (next);
	    }
	    else {
	      // Become the last.
	      Syscall::state ().last_header_ = h;
	    }
	    // Merge with next.
	    h->merge (next);
	  }
	}

	if (h != Syscall::state ().first_header_) {
	  header* prev = h->get_footer ()->prev ()->get_header ();
	  if (prev->available ()) {
	    // Remove from the free list.
	    remove (prev);
	    // Merge.
	    prev->merge (h);
	    if (h == Syscall::state ().last_header_) {
	      // Become the last.
	      Syscall::state ().last_header_ = prev;
	    }
	    h = prev;
	  }
	}

	// Insert.
	if (h != Syscall::state ().last_header_) {
	  insert (h);
	}
      }
    }
  }
  
};

template <class Syscall>
inline void*
operator new (size_t sz,
	      const list_alloc<Syscall>& pa)
{
  return pa.alloc (sz);
}

template <class Syscall>
inline void*
operator new[] (size_t sz,
		const list_alloc<Syscall>& pa)
{
  return pa.alloc (sz);
}

template <class T, class Syscall>
inline void
destroy (T* p,
	 const list_alloc<Syscall>& la)
{
  if (p != 0) {
    p->~T ();
    la.free (p);
  }
}

template <class T, class Syscall>
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
  list_allocator (const list_allocator<U, Syscall>&)
  { }

  static pointer
  allocate (size_type n,
	    std::allocator<void>::const_pointer = 0)
  {
    return static_cast<pointer> (list_alloc<Syscall>::alloc (n * sizeof (T)));
  }
  
  static void
  deallocate (pointer p,
	      size_type)
  {
    list_alloc<Syscall>::free (p);
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
    typedef list_allocator<U, Syscall> other;
  };
};

#endif /* __list_allocator_hpp__ */
