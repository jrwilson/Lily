#include "dymem.h"

/*
  Description
  -----------
  Dynamic memory allocation based on Doug Lea's malloc.
  I would eventually like to port dlmalloc to Lily, however, I'm unwilling to find the Unix assumption right now.

  Authors:
  Justin R. Wilson
*/

/* BUG:  We assume a 32-bit architecture. */

#include <stdbool.h>

/* The number of bins containing free chunks of memory. */
#define BIN_COUNT 128

/* Look up table for hashing chunk size to bin. */
static const size_t bin_size[BIN_COUNT] = {
  0,
  8,
  16,
  24,
  32,
  40,
  48,
  56,
  64,
  72,
  80,
  88,
  96,
  104,
  112,
  120,
  128,
  136,
  144,
  152,
  160,
  168,
  176,
  184,
  192,
  200,
  208,
  216,
  224,
  232,
  240,
  248,
  256,
  264,
  272,
  280,
  288,
  296,
  304,
  312,
  320,
  328,
  336,
  344,
  352,
  360,
  368,
  376,
  384,
  392,
  400,
  408,
  416,
  424,
  432,
  440,
  448,
  456,
  464,
  472,
  480,
  488,
  496,
  504,
  512,
  652,
  831,
  1058,
  1348,
  1717,
  2188,
  2787,
  3550,
  4522,
  5761,
  7339,
  9348,
  11908,
  15170,
  19324,
  24616,
  31357,
  39945,
  50884,
  64819,
  82570,
  105183,
  133988,
  170682,
  217425,
  276969,
  352820,
  449443,
  572527,
  729319,
  929050,
  1183479,
  1507587,
  1920454,
  2446389,
  3116356,
  3969800,
  5056968,
  6441868,
  8206036,
  10453338,
  13316085,
  16962824,
  21608257,
  27525887,
  35064117,
  44666764,
  56899189,
  72481582,
  92331364,
  117617200,
  149827807,
  190859599,
  243128345,
  309711391,
  394528849,
  502574386,
  640209238,
  815536724,
  1038879337,
  1323386482,
  1685808657,
  2147483648U
};

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
typedef struct {
  /* The status is stored in the low bit. */
  size_t size_;
} preamble_t;

/* A free chunk has a normal header and pointers to form a linked list. */
typedef struct header header_t;
struct header {
  preamble_t preamble;
  /* Pointers for the free list. */
  header_t* next;
  header_t* prev;
};

/* Each chunk has a footer containing its size. */
typedef struct {
  size_t size;
} footer_t;

/* Use these sizes when computing linear next and previous. */
#define HEADER_SIZE (sizeof (preamble_t))
#define FOOTER_SIZE (sizeof (footer_t))

/* The smallest unit of allocation corresponds to the pointers in a free header. */
#define ALLOC_MIN (sizeof (header_t) - sizeof (preamble_t))

/* Required alignment for headers. */
#define HEADER_ALIGN (sizeof (header_t) + sizeof (footer_t))

#define AVAILABLE_MASK (1)
#define SIZE_MASK (~(AVAILABLE_MASK))

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
  /* Don't change order without changing header_data (). */
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
  return (void*)&h->next;
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

static header_t* first_header_ = 0;
static header_t* last_header_ = 0;
static header_t* bin_[BIN_COUNT];

void*
adjust_break (size_t size);

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

void*
malloc (size_t size)
{
  if (size == 0) {
    return 0;
  }

  if (first_header_ == 0) {
    /* Initialize. */
    char* original_break = adjust_break (2 * HEADER_ALIGN);
    if (original_break == 0) {
      /* Fail. */
      return 0;
    }
    char* aligned_break = (char*)align_up ((size_t)original_break, HEADER_ALIGN);
    first_header_ = (header_t*)aligned_break;
    header_initialize (first_header_, 2 * HEADER_ALIGN - (aligned_break - original_break) - HEADER_SIZE - FOOTER_SIZE);
    last_header_ = first_header_;
    for (size_t idx = 0; idx != BIN_COUNT; ++idx) {
      bin_[idx] = 0;
    }
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
    size_t request_size = HEADER_SIZE + size + FOOTER_SIZE;
    void* temp = adjust_break (request_size);
    if (temp == 0) {
      /* Fail. */
      return 0;
    }
    last_header_ = temp;
    header_initialize (last_header_, request_size - HEADER_SIZE - FOOTER_SIZE);
  }
  else if (header_size (last_header_) < size) {
    // The last chunk is available but too small.
    // Resize the last chunk.
    size_t request_size = size - header_size (last_header_);
    if (adjust_break (request_size) == 0) {
      /* Fail. */
      return 0;
    }
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

  /* Success. */
  return header_data (ptr);
}

void
free (void* ptr)
{
  if (ptr == 0) {
    return;
  }

  header_t* h = (header_t*) ((char*)(ptr) - HEADER_SIZE);

  if (h >= first_header_ &&
      h <= last_header_ &&
      !header_get_available (h)) {
    
    footer_t* f = header_get_footer (h);
    
    if (f >= header_get_footer (first_header_) &&
	f <= header_get_footer (last_header_) &&
	header_size (h) == footer_size (f)) {
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
  }
}

/*   // static void */
/*   // check () */
/*   // { */
/*   //   // Scan the free list. */
/*   //   size_t free = 0; */
/*   //   for (size_t idx = 0; idx < BIN_COUNT; ++idx) { */
/*   //     header* h = bin_[idx]; */
/*   //     while (h != 0) { */
/*   // 	kassert (first_header_ <= h); */
/*   // 	kassert (h < last_header_); */
/*   // 	kassert (h->available ()); */
/*   // 	++free; */
/*   // 	kassert (h->size () >= ALLOC_MIN); */
/*   // 	footer* f = h->get_footer (); */
/*   // 	kassert (h->size () == f->size ()); */
/*   // 	kassert (f->get_header () == h); */

/*   // 	header* t; */
/*   // 	t = h->free_next (); */
/*   // 	kassert (t == 0 || (first_header_ <= t && t < last_header_)); */
/*   // 	t = h->free_prev (); */
/*   // 	kassert (t == 0 || (first_header_ <= t && t < last_header_)); */

/*   // 	h = h->free_next (); */
/*   //     } */
/*   //   } */

/*   //   // Scan all of the chunks. */
/*   //   size_t chunks = 0; */
/*   //   size_t available = 0; */
/*   //   for (header* ptr = first_header_; ptr <= last_header_; ptr = ptr->next ()) { */
/*   //     ++chunks; */
/*   //     if (ptr->available ()) { */
/*   // 	++available; */
/*   //     } */
/*   //   } */
/*   //   kassert (free <= available); */
/*   //   kassert (available <= chunks); */
/*   //   kout << "\t" << free << "/" << available << "/" << chunks << endl; */
/*   // } */
