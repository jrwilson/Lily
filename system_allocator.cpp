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

system_alloc::chunk_header* system_alloc::first_header_ = 0;
system_alloc::chunk_header* system_alloc::last_header_ = 0;
logical_address_t system_alloc::heap_end_ = 0;
bool system_alloc::backing_ = false;

void*
system_alloc::allocate (size_t size)
{
  // Page aligment makes mapping easier.
  kassert (is_aligned (size, PAGE_SIZE));
  logical_address_t retval = reinterpret_cast<logical_address_t> (last_header_ + 1) + last_header_->size;
  // Check to make sure we don't run out of logical address space.
  kassert (retval + size <= heap_end_);
  if (backing_) {
    // Back with frames.
    for (size_t x = 0; x != size; x += PAGE_SIZE) {
      vm::map (retval + x, frame_manager::alloc (), vm::USER, vm::WRITABLE);
    }
  }
  
  return reinterpret_cast<void*> (retval);
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
