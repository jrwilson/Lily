#ifndef __kernel_allocator_hpp__
#define __kernel_allocator_hpp__

/*
  File
  ----
  kernel_allocator.hpp
  
  Description
  -----------
  Memory allocator based on Doug Lea's malloc.

  Authors:
  Justin R. Wilson
*/

#include "vm_def.hpp"
#include "kassert.hpp"
#include "new.hpp"

// TODO:  We assume a 32-bit architecture.

class kernel_alloc {
private:
  /* The number of bins containing free chunks of memory. */
  static const size_t BIN_COUNT = 128;

  /* Look up table for hashing chunk size to bin. */
  static const size_t bin_size[BIN_COUNT];

  /* Each chunk of allocated memory has a header containing its size and status and a footer containing its size.
     Thus, memory looks like:
     header1 data1 footer1 header2 data2 footer2 etc.
     
     Technically, there are two kinds of headers corresponding to the availablility of the chunk.
     Free chunks contain two pointers after the size that allows them to be inserted into a doubly linked list.
     Thus, the minimum allocation size is the size of two pointers.
     
     A chunk always has a linear next and previous based on it physical location.
     When computing the linear and next, one must be careful to take into account the pointers.
  */
  
  /* Each chunk of allocated memory has a header containing its size and status. */
  struct preamble_t {
    /* The status is stored in the low bit. */
    size_t size_;
  };
  
  /* A free chunk has a normal header and pointers to form a linked list. */
  struct header_t {
    preamble_t preamble;
    /* Pointers for the free list. */
    header_t* next;
    header_t* prev;
  };
  
  /* Each chunk has a footer containing its size. */
  struct footer_t {
    size_t size;
  };
  
  /* Use these sizes when computing linear next and previous. */
  static const size_t HEADER_SIZE = sizeof (preamble_t);
  static const size_t FOOTER_SIZE = sizeof (footer_t);
  
  /* The smallest unit of allocation corresponds to the pointers in a free header. */
  static const size_t ALLOC_MIN = sizeof (header_t) - sizeof (preamble_t);
  
  /* Required alignment for headers. */
  static const size_t HEADER_ALIGN = sizeof (header_t) + sizeof (footer_t);
  
  static const size_t AVAILABLE_MASK = 1;
  static const size_t SIZE_MASK = ~(AVAILABLE_MASK);
  
  static size_t
  preamble_get_size (preamble_t p)
  {
    return p.size_ & SIZE_MASK;
  }
  
  static void
  preamble_set_size (preamble_t* p,
		     size_t size)
  {
    p->size_ = size | (p->size_ & AVAILABLE_MASK);
  }
  
  static bool
  preamble_get_available (preamble_t p)
  {
    return p.size_ & AVAILABLE_MASK;
  }
  
  static void
  preamble_set_available (preamble_t* p,
			  bool available)
  {
    p->size_ = (p->size_ & SIZE_MASK) | available;
  }
  
  static inline void
  footer_initialize (footer_t* f,
		     size_t size)
  {
    f->size = size;
  }
  
  static inline size_t
  footer_size (const footer_t* f)
  {
    return f->size;
  }
  
  static inline header_t*
  footer_get_header (const footer_t* f)
  {
    return (header_t*) ((char*)(f) - f->size - HEADER_SIZE);
  }
  
  static inline footer_t*
  footer_prev (const footer_t* f)
  {
    return (footer_t*) ((char*)(f) - f->size - HEADER_SIZE - FOOTER_SIZE);
  }
  
  static inline footer_t*
  header_get_footer (const header_t* h)
  {
    /* Skip the header and the data. */
    return (footer_t*) ((char*)(h) + HEADER_SIZE + preamble_get_size (h->preamble));
  }
  
  static inline void
  header_initialize (header_t* h,
		     size_t size)
  {
    preamble_set_size (&h->preamble, size);
    preamble_set_available (&h->preamble, true);
    h->next = 0;
    h->prev = 0;
    /* Must be after we set size. */
    footer_initialize (header_get_footer (h), size);
  }
  
  static inline bool
  header_get_available (const header_t* h)
  {
    return preamble_get_available (h->preamble);
  }
  
  static inline void
  header_set_available (header_t* h,
			bool a)
  {
    preamble_set_available (&h->preamble, a);
  }
  
  static inline size_t
  header_size (const header_t* h)
  {
    return preamble_get_size (h->preamble);
  }
  
  static inline header_t*
  header_next (const header_t* h)
  {
  /* Skip over the header, the data, and the footer. */
    return (header_t*) ((char*)(h) + HEADER_SIZE + preamble_get_size (h->preamble) + FOOTER_SIZE);
  }
  
  /* Merge with the next chunk. */
  static inline void
  header_merge (header_t* h,
		const header_t* n)
  {
    preamble_set_size (&h->preamble, preamble_get_size (h->preamble) + FOOTER_SIZE + HEADER_SIZE + preamble_get_size (n->preamble));
    footer_initialize (header_get_footer (h), preamble_get_size (h->preamble));
  }
  
  static inline void*
  header_data (const header_t* h)
  {
    return ((char*)h) + HEADER_SIZE;
  }
  
  static inline header_t*
  header_split (header_t* h,
		size_t size)
  {
    /*
      Ignore the pointers in header_t.
      
      Before the split: header_t size1                          footer_t
      After  the split: header_t size footer_t | header_t size2 footer_t
      The two equations are:
      size1 == size + FOOTER_SIZE + HEADER_SIZE + size2
      size2 >= ALLOC_MIN
      Solve for size2.
      size2 == size1 - size - FOOTER_SIZE - HEADER_SIZE
      Substitute.
      size1 - size - FOOTER_SIZE - HEADER_SIZE >= ALLOC_MIN
      Rearrange.
      size1 >= ALLOC_MIN + size + FOOTER_SIZE + HEADER_SIZE
    */
    
    if (!(preamble_get_size (h->preamble) >= ALLOC_MIN + size + FOOTER_SIZE + HEADER_SIZE)) {
      return 0;
    }
    else {
      header_t* n = (header_t*) ((char*)(h) + HEADER_SIZE + size + FOOTER_SIZE);
      header_initialize (n, preamble_get_size (h->preamble) - size - FOOTER_SIZE - HEADER_SIZE);
      header_initialize (h, size);
      return n;
    }
  }
  
  static inline size_t
  align_up (size_t value,
	    size_t radix)
  {
    return (value + radix - 1) & ~(radix - 1);
  }
  
  static inline size_t
  max (size_t x,
       size_t y)
  {
    return (x > y) ? x : y;
  }
  
  static inline size_t
  size_to_bin (size_t size)
  {
    if (size <= 512) {
      return size / 8;
    }
    else {
      /* TODO:  Possibly use binary search instead of linear search. */
      size_t idx;
      for (idx = 512 / 8; idx < BIN_COUNT - 1; ++idx) {
	if (bin_size[idx] <= size && size < bin_size[idx + 1]) {
	  break;
	}
      }
      return idx;
    }
  }
  
  static void*
  sbrk (size_t size);

  static inline void
  insert (header_t* h)
  {
    header_t* prev;
    header_t* next;
    const size_t idx = size_to_bin (header_size (h));
    for (prev = 0, next = bin_[idx];
	 next != 0 && (header_size (next) < header_size (h) || next < h);
	 prev = next, next = next->next) ;;
    
    if (next != 0) {
      next->prev = h;
    }
    
    h->next = next;
    h->prev = prev;
    
    if (prev != 0) {
      prev->next = h;
    }
    else {
      bin_[idx] = h;
    }
  }
  
  static inline void
  remove (header_t* h)
  {
    header_t* prev = h->prev;
    header_t* next = h->next;
    
    if (prev != 0) {
      prev->next = next;
    }
    else {
      bin_[size_to_bin (header_size (h))] = next;
    }
    
    if (next != 0) {
      next->prev = prev;
    }
  }
  
  static logical_address_t heap_begin_;
  static logical_address_t heap_end_;
  static logical_address_t heap_limit_;
  static bool backing_;
  static header_t* first_header_;
  static header_t* last_header_;
  static header_t* bin_[BIN_COUNT];

public:

  static void
  initialize (logical_address_t begin,
	      logical_address_t limit)
  {
    // We use a page as the initial size of the heap.
    kassert (begin < limit);
    kassert (limit - begin >= PAGE_SIZE);
    
    heap_begin_ = begin;
    heap_end_ = begin;
    heap_limit_ = limit;
    backing_ = false;
    
    first_header_ = static_cast<header_t*> (sbrk (PAGE_SIZE));
    header_initialize (first_header_, PAGE_SIZE - HEADER_SIZE - FOOTER_SIZE);
    last_header_ = first_header_;
    for (size_t idx = 0; idx != BIN_COUNT; ++idx) {
      bin_[idx] = 0;
    }
  }

  static logical_address_t
  heap_begin ()
  {
    return heap_begin_;
  }
  
  static logical_address_t
  heap_end ()
  {
    return heap_end_;
  }
  
  static void
  engage_vm (logical_address_t limit)
  {
    heap_limit_ = limit;
    backing_ = true;
  }
  
  static inline void*
  alloc (size_t size)
  {
    if (size == 0) {
      return 0;
    }

    // Increase the size to the minimum and align.
    size = align_up (max (size, ALLOC_MIN), ALLOC_MIN);
    
    // Try to allocate a chunk from the list of free chunks.
    for (size_t idx = size_to_bin (size); idx != BIN_COUNT; ++idx) {
      for (header_t* ptr = bin_[idx]; ptr != 0; ptr = ptr->next) {
	if (header_size (ptr) >= size) {
	  // Remove from the free list.
	  remove (ptr);
	  // Try to split.
	  header_t* s = header_split (ptr, size);
	  if (s != 0) {
	    insert (s);
	  }
	  // Mark as used and return.
	  header_set_available (ptr, false);
	  return header_data (ptr);
	}
      }
    }
    
    // Allocating a chunk from the free list failed.
    // Use the last chunk.
    if (!header_get_available (last_header_)) {
      // The last chunk is not available.
      // Create one.
      size_t request_size = align_up (HEADER_SIZE + size + FOOTER_SIZE, PAGE_SIZE);
      last_header_ = static_cast<header_t*> (sbrk (request_size));
      header_initialize (last_header_, request_size - HEADER_SIZE - FOOTER_SIZE);
    }
    else if (header_size (last_header_) < size) {
      // The last chunk is available but too small.
      // Resize the last chunk.
      size_t request_size = align_up (size - header_size (last_header_), PAGE_SIZE);
      sbrk (request_size);
      header_initialize (last_header_, header_size (last_header_) + request_size);
    }
    
    // The last chunk is now of sufficient size.  Try to split.
    header_t* ptr = last_header_;
    header_t* s = header_split (ptr, size);
    if (s != 0) {
      last_header_ = s;
    }
    
    // Mark as used and return.
    header_set_available (ptr, false);

    return header_data (ptr);
  }
  
  static inline void
  free (void* ptr)
  {
    if (ptr == 0) {
      return;
    }
    
    header_t* h = (header_t*) ((char*)(ptr) - HEADER_SIZE);

    kassert (h >= first_header_);
    kassert (h <= last_header_);
    kassert (!header_get_available (h));
    
    footer_t* f = header_get_footer (h);
    
    kassert (f >= header_get_footer (first_header_));
    kassert (f <= header_get_footer (last_header_));
    kassert (header_size (h) == footer_size (f));

    /* We are satisfied that the chunk is correct. */
    /* We could be more paranoid and check the previous/next chunk. */
    
    // Chunk is now available.
    header_set_available (h, true);

    if (h != last_header_) {
      header_t* next = header_next (h);
      if (header_get_available (next)) {
	if (next != last_header_) {
	  // Remove from the free list.
	  remove (next);
	}
	else {
	  // Become the last.
	  last_header_ = h;
	}
	// Merge with next.
	header_merge (h, next);
      }
    }
    
    if (h != first_header_) {
      header_t* prev = footer_get_header (footer_prev (header_get_footer (h)));
      if (header_get_available (prev)) {
	// Remove from the free list.
	remove (prev);
	// Merge.
	header_merge (prev, h);
	if (h == last_header_) {
	  // Become the last.
	  last_header_ = prev;
	}
	h = prev;
      }
    }
    
    // Insert.
    if (h != last_header_) {
      insert (h);
    }
  }

  static void
  check ()
  {
    kassert (heap_begin_ < heap_end_);
    kassert (heap_end_ <= heap_limit_);

    kassert (first_header_ <= last_header_);
    kassert (first_header_ == reinterpret_cast<header_t*> (heap_begin_));
    kassert (last_header_ < reinterpret_cast<header_t*> (heap_end_));

    footer_t* first_footer = header_get_footer (first_header_);
    footer_t* last_footer = header_get_footer (last_header_);
    kassert (footer_get_header (first_footer) == first_header_);
    kassert (footer_get_header (last_footer) == last_header_);

    // Scan the free list.
    size_t free = 0;
    for (size_t idx = 0; idx < BIN_COUNT; ++idx) {
      header_t* h = bin_[idx];
      while (h != 0) {
  	kassert (first_header_ <= h);
  	kassert (h < last_header_);
	kassert (header_get_available (h));
  	kassert (header_size (h) >= ALLOC_MIN);

  	footer_t* f = header_get_footer (h);
	kassert (first_footer <= f);
	kassert (f < last_footer);
  	kassert (header_size (h) == footer_size (f));
  	kassert (footer_get_header (f) == h);

  	header_t* next = h->next;
  	kassert (next == 0 || (first_header_ <= next && next < last_header_));
  	header_t* prev = h->prev;
  	kassert (prev == 0 || (first_header_ <= prev && prev < last_header_));

	if (next != 0) {
	  kassert (next->prev == h);
	}
	if (prev != 0) {
	  kassert (prev->next == h);
	}

  	++free;

  	h = h->next;
      }
    }

    // Scan the heap.
    size_t total = 0;
    size_t available = 0;
    header_t* h;
    bool last_header_on_free_list = false;
    for (h = first_header_; h <= last_header_; h = header_next (h)) {
      kassert (h >= first_header_);
      kassert (h <= last_header_);
      kassert (header_size (h) >= ALLOC_MIN);

      footer_t* f = header_get_footer (h);
      kassert (first_footer <= f);
      kassert (f <= last_footer);
      kassert (header_size (h) == footer_size (f));
      kassert (footer_get_header (f) == h);

      ++total;

      if (header_get_available (h)) {
	++available;

	// This should appear exactly once in the correct bin.
	size_t count = 0;
	for (size_t idx = 0; idx < BIN_COUNT; ++idx) {
	  header_t* f = bin_[idx];
	  while (f != 0) {
	    if (f == h) {
	      ++count;
	      kassert (idx == size_to_bin (header_size (h)));
	    }
	    f = f->next;
	  }
	}

	if (h != last_header_) {
	  kassert (count == 1);
	}
	else {
	  kassert (count == 1 || count == 0);
	  if (count == 1) {
	    last_header_on_free_list = true;
	  }
	}

      }
    }

    kassert (h == reinterpret_cast<header_t*> (heap_end_));
    if (last_header_on_free_list) {
      kassert (free == available);
    }
    else {
      if (header_get_available (last_header_)) {
	kassert (free == available - 1);
      }
      else {
	kassert (free == available);
      }
    }

    kassert (available <= total);
    kout << "free = " << free << " available = " << available << " total = " << total << endl;
  }
  
};

#endif /* __kernel_allocator_hpp__ */
