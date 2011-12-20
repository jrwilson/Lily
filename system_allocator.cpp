/*
  File
  ----
  system_allocator.cpp
  
  Description
  -----------
  Memory allocator using a doubly-linked list.

  Authors:
  Justin R. Wilson
*/

#include "system_allocator.hpp"
#include "frame_manager.hpp"
#include "vm.hpp"
#include <new>
#include "kassert.hpp"
#include "system_automaton.hpp"

static const uint32_t MAGIC = 0x7ACEDEAD;

struct system_alloc::chunk_header {
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

system_alloc::chunk_header* system_alloc::first_header_ = 0;
system_alloc::chunk_header* system_alloc::last_header_ = 0;
const void* system_alloc::heap_end_ = 0;

system_alloc::chunk_header*
system_alloc::find_header (chunk_header* start,
			 size_t size)
{
  for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
  return start;
}

void
system_alloc::split_header (chunk_header* ptr,
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
system_alloc::merge_header (chunk_header* ptr)
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

void*
system_alloc::allocate (size_t size)
{
  // Page aligment makes mapping easier.
  kassert (is_aligned (size, PAGE_SIZE));
  if (system_automaton::system_automaton != 0) {
    void* retval = system_automaton::system_automaton->sbrk (size);
    kassert (retval != 0);
    // Back with frames.
    for (size_t x = 0; x < size; x += PAGE_SIZE) {
      vm::map (static_cast<uint8_t*> (retval) + x, frame_manager::alloc (), vm::USER, vm::WRITABLE);
    }
    return retval;
  }
  else {
    void* retval = reinterpret_cast<uint8_t*> (last_header_ + 1) + last_header_->size;
    // Check to make sure we don't run out of memory.
    kassert (static_cast<uint8_t*> (retval) + size <= heap_end_);
    return retval;
  }
}

void
system_alloc::initialize (void* begin,
			  const void* end)
{
  // We use a page as the initial size of the heap.
  const size_t request_size = PAGE_SIZE;
  kassert (begin < end);
  kassert (reinterpret_cast<size_t> (end) - reinterpret_cast<size_t> (begin) >= request_size);

  first_header_ = new (begin) chunk_header (request_size - sizeof (chunk_header));
  last_header_ = first_header_;
  heap_end_ = end;
}

const void*
system_alloc::heap_begin ()
{
  return first_header_;
}

size_t
system_alloc::heap_size ()
{
  return (reinterpret_cast<uint8_t*> (last_header_ + 1) + last_header_->size) - reinterpret_cast<uint8_t*> (first_header_);
}

void*
system_alloc::alloc (size_t size)
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

void
system_alloc::free (void* p)
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
// system_allocator_dump (system_allocator_t* la)
// {
//   kputs ("first = "); kputp (la->first_header); kputs (" last = "); kputp (la->last_header); kputs ("\n");
//   kputs ("Node       Magic      Size       Prev       Next\n");
//   dump_heap_int (la->first_header);
// }

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
