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
#include "new.hpp"

/* This will need to be removed later. */
#include "kassert.hpp"
#include "vm_manager.hpp"

const uint32_t MAGIC = 0x7ACEDEAD;

struct list_allocator::chunk_header {
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

list_allocator::list_allocator ()
{
  page_size_ = sys_get_page_size ();
  size_t request_size = physical_address (sizeof (list_allocator) + sizeof (chunk_header)).align_up (page_size_).value ();

  first_header_ = new ((logical_address (this) + sizeof (list_allocator)).value ()) chunk_header (request_size - sizeof (list_allocator) - sizeof (chunk_header));
  last_header_ = first_header_;
}

void*
list_allocator::operator new (size_t)
{
  size_t page_size = sys_get_page_size ();
  size_t request_size = physical_address (sizeof (list_allocator) + sizeof (chunk_header)).align_up (page_size).value ();
  return sys_allocate (request_size);
}

void
list_allocator::operator delete (void*)
{
  kassert (0);
}

list_allocator::chunk_header*
list_allocator::find_header (chunk_header* start,
			     size_t size)
{
  for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
  return start;
}

void
list_allocator::split_header (chunk_header* ptr,
			      size_t size)
{
  kassert (ptr != 0);
  kassert (ptr->available);
  kassert (ptr->size > sizeof (chunk_header) + size);

  /* Split the block. */
  chunk_header* n = new ((logical_address (ptr) + sizeof (chunk_header) + size).value ()) chunk_header (ptr->size - size - sizeof (chunk_header));
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

void*
list_allocator::alloc (size_t size)
{
  chunk_header* ptr = find_header (first_header_, size);

  /* Acquire more memory. */
  if (ptr == 0) {
    /* TODO */
    kassert (0);
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
list_allocator::free (void* p)
{
  chunk_header* ptr = static_cast<chunk_header*> (p);
  --ptr;
  kassert (ptr >= first_header_);
  kassert (ptr <= last_header_);
  kassert (ptr->magic == MAGIC);
  kassert (ptr->available == 0);

  ptr->available = 1;
  
  /* Merge with next. */
  if (ptr->next != 0 && ptr->next->available && ptr->next == (chunk_header*)((char*)ptr + sizeof (chunk_header) + ptr->size)) {
    ptr->size += sizeof (chunk_header) + ptr->next->size;
    if (ptr->next == last_header_) {
      last_header_ = ptr;
    }
    ptr->next = ptr->next->next;
    ptr->next->prev = ptr;
  }
  
  /* Merge with prev. */
  if (ptr->prev != 0 && ptr->prev->available && ptr == (chunk_header*)((char*)ptr->prev + sizeof (chunk_header) + ptr->prev->size)) {
    ptr->prev->size += sizeof (chunk_header) + ptr->size;
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
    if (ptr == last_header_) {
      last_header_ = ptr->prev;
    }
  }
}

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
