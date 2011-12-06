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
#include "vm_manager.hpp"
#include <new>
#include "kassert.hpp"
#include "vm_area.hpp"

// A call to alloc () might call automaton->alloc ().
// automaton->alloc () might call alloc () to create data structures.
// The recursive calls must succeed without calling automaton->alloc (), otherwise, infinite recursion.
// If we can assume an upper bound on the memory allocated by the recursive call, then we can ensure the recursive calls will succeed by having that must extra memory on hand.
// THE CURRENT IMPLEMENTATION DOES NOT MEET THIS REQUIREMENT.
// Mainly because it uses a dynamic array.
// In order to meet this requirement, one needs to use something like a list or tree where inserting can be performed in constant space.

static const size_t RESERVE_AMOUNT = PAGE_SIZE;

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

system_alloc::chunk_header*
system_alloc::find_header (chunk_header* start,
			 size_t size) const
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
  kassert (is_aligned (size, PAGE_SIZE));

  switch (status_) {
  case NORMAL:
    {
      return automaton_->alloc (size);
    }
    break;
  case BOOTING:
    {
      void* retval = const_cast<void*> (static_cast<const void*> (boot_end_));
      boot_end_ += size;
      boot_end_ = static_cast<uint8_t*> (const_cast<void*> (align_up (boot_end_, PAGE_SIZE)));
      // Back the memory with frames to avoid page faults when the memory map is incomplete.
      // Page faults are okay later.
      for (uint8_t* address = static_cast<uint8_t*> (retval); address < boot_end_; address += PAGE_SIZE) {
	vm_manager::map (address, frame_manager::alloc (), paging_constants::SUPERVISOR, paging_constants::WRITABLE);
      }
      return retval;
    }
    break;
  }

  // The system automaton allocated memory during an operation.
  kassert (0);
  return 0;
}

void
system_alloc::boot (const void* boot_begin,
		    const void* boot_end)
{
  status_ = BOOTING;
  boot_begin_ = static_cast<uint8_t*> (const_cast<void*> (align_down (boot_begin, PAGE_SIZE)));
  boot_end_ = static_cast<uint8_t*> (const_cast<void*> (align_up (boot_end, PAGE_SIZE)));
}

void
system_alloc::normal (automaton_interface* a)
{
  kassert (a != 0);
  automaton_ = a;
  size_t request_size = align_up (sizeof (chunk_header) + RESERVE_AMOUNT, PAGE_SIZE);
  first_header_ = new (allocate (request_size)) chunk_header (request_size - sizeof (chunk_header));
  last_header_ = first_header_;

  // Finish the memory map for the system automaton with data that has been allocated.
  // We use a fixed-point style because inserting might cause the end to move.
  void* boot_end_before;
  while (true) {
    boot_end_before = boot_end_;
    vm_data_area* d = new (alloc (sizeof (vm_data_area))) vm_data_area (boot_begin_, boot_end_, paging_constants::SUPERVISOR);
    kassert (automaton_->insert_vm_area (d));
    if (boot_end_before == boot_end_) {
      break;
    }
    else {
      automaton_->remove_vm_area (d);
    }
  }
  status_ = NORMAL;
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
    size_t request_size = align_up (sizeof (chunk_header) + size + RESERVE_AMOUNT, PAGE_SIZE);
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
