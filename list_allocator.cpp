/*
  File
  ----
  list_allocator.cpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list.

  Authors:
  Justin R. Wilson
*/

#include "list_allocator.hpp"
#include "syscall.hpp"
#include <new>

static const uint32_t MAGIC = 0x7ACEDEAD;

struct list_alloc::chunk_header {
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

list_alloc::chunk_header*
list_alloc::find_header (chunk_header* start,
			 size_t size)
{
  for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
  return start;
}

void
list_alloc::split_header (chunk_header* ptr,
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

// Try to merge with the next chunk.
bool
list_alloc::merge_header (chunk_header* ptr)
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


list_alloc::list_alloc (bool initialize)
{
  if (initialize) {
    page_size_ = sys_get_page_size ();
    first_header_ = new (sys_allocate (page_size_)) chunk_header (page_size_ - sizeof (chunk_header));
    last_header_ = first_header_;
  }
}

static inline size_t
align_up (size_t value,
	  size_t radix)
{
  return (value + radix - 1) & ~(radix - 1);
}


void*
list_alloc::alloc (size_t size)
{
  if (size == 0) {
    return 0;
  }

  chunk_header* ptr = find_header (first_header_, size);

  /* Acquire more memory. */
  if (ptr == 0) {
    size_t request_size = align_up (sizeof (chunk_header) + size, page_size_);
    ptr = new (sys_allocate (request_size)) chunk_header (request_size - sizeof (chunk_header));

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

void
list_alloc::free (void* p)
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

// static void
// dump_heap_int (chunk_header* ptr)
// {
//   if (ptr != 0) {
//     kputp (ptr); kputs (" "); kputx32 (ptr->magic); kputs (" "); kputx32 (ptr->size); kputs (" "); kputp (ptr->prev); kputs (" "); kputp (ptr->next);
//     if (ptr->available) {
//       kputs (" Free\n");
//     }
//     else {
//       kputs (" Used\n");
//     }

//     dump_heap_int (ptr->next);
//   }
// }

// static void
// list_allocator_dump (list_allocator_t* la)
// {
//   kputs ("first = "); kputp (la->first_header); kputs (" last = "); kputp (la->last_header); kputs ("\n");
//   kputs ("Node       Magic      Size       Prev       Next\n");
//   dump_heap_int (la->first_header);
// }

/* static void */
/* expand_heap (unsigned int size) */
/* { */
/*   while (heap_last->size < size) { */
/*     /\* Map it. *\/ */
/*     //insert_mapping (&kernel_page_directory, heap_limit, make_page_table_entry (frame_manager_allocate (), NOT_GLOBAL, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT), make_page_directory_entry (0, PAGE_SIZE_4K, CACHED, WRITE_BACK, SUPERVISOR, WRITABLE, PRESENT)); */
/*     /\* Update sizes and limits. *\/ */
/*     heap_limit += PAGE_SIZE; */
/*     heap_last->size += PAGE_SIZE; */

/*     /\* Replace the spare page table if necessary. *\/ */
/*     if (spare_page_table == 0) { */
/*       spare_page_table = allocate_page_table (); */
/*     } */
/*   } */
/* } */


/* static chunk_header* */
/* find_header_pa (chunk_header* start, */
/* 		unsigned int size, */
/* 		unsigned int* base, */
/* 		unsigned int* limit) */
/* { */
/*   while (start != 0) { */
/*     if (start->available && start->size >= size) { */
/*       *base = PAGE_ALIGN ((unsigned int)start + sizeof (chunk_header) + PAGE_SIZE - 1); */
/*       *limit = (unsigned int)start + sizeof (chunk_header) + start->size; */
/*       if ((*limit - *base) >= size) { */
/* 	break; */
/*       } */
/*     } */
/*     start = start->next; */
/*   } */
/*   return start; */
/* } */

/* void* */
/* kmalloc_pa (unsigned int size) */
/* { */
/*   unsigned int base; */
/*   unsigned int limit; */

/*   chunk_header* ptr = find_header_pa (heap_first, size, &base, &limit); */
/*   while (ptr == 0) { */
/*     expand_heap (size); */
/*     ptr = find_header_pa (heap_last, size, &base, &limit); */
/*   } */

/*   if (((unsigned int)ptr + sizeof (chunk_header)) != base) { */
/*     if ((base - sizeof (chunk_header)) > ((unsigned int)ptr + sizeof (chunk_header))) { */
/*       /\* Split. *\/ */
/*       split (ptr, ptr->size - sizeof (chunk_header) - (limit - base)); */
/*       /\* Change ptr to aligned block. *\/ */
/*       ptr = ptr->next; */
/*     } */
/*     else { */
/*       /\* Slide the header to align it with base. *\/ */
/*       unsigned int amount = base - ((unsigned int)ptr + sizeof (chunk_header)); */
/*       chunk_header* p = ptr->prev; */
/*       chunk_header* n = ptr->next; */
/*       unsigned int s = ptr->size; */
/*       chunk_header* old = ptr; */
      
/*       ptr = (chunk_header*) ((unsigned int)ptr + amount); */
/*       ptr->available = 1; */
/*       ptr->magic = HEAP_MAGIC; */
/*       ptr->size = s - amount; */
/*       ptr->prev = p; */
/*       ptr->next = n; */
      
/*       if (ptr->prev != 0) { */
/* 	ptr->prev->next = ptr; */
/* 	ptr->prev->size += amount; */
/*       } */
      
/*       if (ptr->next != 0) { */
/* 	ptr->next->prev = ptr; */
/*       } */
      
/*       if (heap_first == old) { */
/* 	heap_first = ptr; */
/*       } */
      
/*       if (heap_last == old) { */
/* 	heap_last = ptr; */
/*       } */
/*     } */
/*   } */
  
/*   if ((ptr->size - size) > sizeof (chunk_header)) { */
/*     split (ptr, size); */
/*   } */
/*   /\* Mark as used and return. *\/ */
/*   ptr->available = 0; */
/*   return (ptr + 1); */
/* } */
