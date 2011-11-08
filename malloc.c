/*
  File
  ----
  malloc.c
  
  Description
  -----------
  Memory allocator.

  Authors:
  Justin R. Wilson
*/

#include "malloc.h"
/* This will need to be removed later. */
#include "kassert.h"
#include "syscall.h"

#define MAGIC 0x7ACEDEAD

typedef struct header header_t;
struct header {
  uint32_t available : 1;
  uint32_t magic : 31;
  size_t size; /* Does not include header. */
  header_t* prev;
  header_t* next;
};

static size_t page_size = 0;
static header_t* first_header = 0;
static header_t* last_header = 0;

static header_t*
find_header (header_t* start,
	     size_t size)
{
  for (; start != 0 && !(start->available && start->size >= size); start = start->next) ;;
  return start;
}

static void
split (header_t* ptr,
       size_t size)
{
  kassert (ptr != 0);
  kassert (ptr->available);
  kassert (ptr->size > sizeof (header_t) + size);

  /* Split the block. */
  header_t* n = (header_t*)((char*)ptr + sizeof (header_t) + size);
  n->available = 1;
  n->magic = MAGIC;
  n->size = ptr->size - size - sizeof (header_t);
  n->prev = ptr;
  n->next = ptr->next;
  
  ptr->size = size;
  ptr->next = n;
  
  if (n->next != 0) {
    n->next->prev = n;
  }
  
  if (ptr == last_header) {
    last_header = n;
  }
}

static void
dump_heap_int (header_t* ptr)
{
  if (ptr != 0) {
    kputp (ptr); kputs (" "); kputx32 (ptr->magic); kputs (" "); kputx32 (ptr->size); kputs (" "); kputp (ptr->prev); kputs (" "); kputp (ptr->next);
    if (ptr->available) {
      kputs (" Free\n");
    }
    else {
      kputs (" Used\n");
    }

    dump_heap_int (ptr->next);
  }
}

static void
dump_heap ()
{
  kputs ("first = "); kputp (first_header); kputs (" last = "); kputp (last_header); kputs ("\n");
  kputs ("Node       Magic      Size       Prev       Next\n");
  dump_heap_int (first_header);
}

void*
malloc (size_t size)
{
  if (page_size == 0) {
    /* Initialize. */
    page_size = sys_get_page_size ();
    first_header = sys_allocate (page_size);
    last_header = first_header;
    first_header->available = 1;
    first_header->magic = MAGIC;
    first_header->size = page_size - sizeof (header_t);
    first_header->prev = 0;
    first_header->next = 0;
  }

  header_t* ptr = find_header (first_header, size);
  if (ptr == 0) {
    /* TODO */
    kassert (0);
  }

  /* Found something we can use. */
  if ((ptr->size - size) > sizeof (header_t)) {
    split (ptr, size);
  }
  /* Mark as used and return. */
  ptr->available = 0;
  return (ptr + 1);
}

void
free (void* p)
{
  header_t* ptr = p;
  --ptr;
  if (p != 0 &&
      ptr >= first_header &&
      ptr <= last_header &&
      ptr->magic == MAGIC &&
      ptr->available == 0) {

    ptr->available = 1;
    
    /* Merge with next. */
    if (ptr->next != 0 && ptr->next->available && ptr->next == (header_t*)((char*)ptr + sizeof (header_t) + ptr->size)) {
      ptr->size += sizeof (header_t) + ptr->next->size;
      if (ptr->next == last_header) {
    	last_header = ptr;
      }
      ptr->next = ptr->next->next;
      ptr->next->prev = ptr;
    }
    
    /* Merge with prev. */
    if (ptr->prev != 0 && ptr->prev->available && ptr == (header_t*)((char*)ptr->prev + sizeof (header_t) + ptr->prev->size)) {
      ptr->prev->size += sizeof (header_t) + ptr->size;
      ptr->prev->next = ptr->next;
      ptr->next->prev = ptr->prev;
      if (ptr == last_header) {
    	last_header = ptr->prev;
      }
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


/* static header_t* */
/* find_header_pa (header_t* start, */
/* 		unsigned int size, */
/* 		unsigned int* base, */
/* 		unsigned int* limit) */
/* { */
/*   while (start != 0) { */
/*     if (start->available && start->size >= size) { */
/*       *base = PAGE_ALIGN ((unsigned int)start + sizeof (header_t) + PAGE_SIZE - 1); */
/*       *limit = (unsigned int)start + sizeof (header_t) + start->size; */
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

/*   header_t* ptr = find_header_pa (heap_first, size, &base, &limit); */
/*   while (ptr == 0) { */
/*     expand_heap (size); */
/*     ptr = find_header_pa (heap_last, size, &base, &limit); */
/*   } */

/*   if (((unsigned int)ptr + sizeof (header_t)) != base) { */
/*     if ((base - sizeof (header_t)) > ((unsigned int)ptr + sizeof (header_t))) { */
/*       /\* Split. *\/ */
/*       split (ptr, ptr->size - sizeof (header_t) - (limit - base)); */
/*       /\* Change ptr to aligned block. *\/ */
/*       ptr = ptr->next; */
/*     } */
/*     else { */
/*       /\* Slide the header to align it with base. *\/ */
/*       unsigned int amount = base - ((unsigned int)ptr + sizeof (header_t)); */
/*       header_t* p = ptr->prev; */
/*       header_t* n = ptr->next; */
/*       unsigned int s = ptr->size; */
/*       header_t* old = ptr; */
      
/*       ptr = (header_t*) ((unsigned int)ptr + amount); */
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
  
/*   if ((ptr->size - size) > sizeof (header_t)) { */
/*     split (ptr, size); */
/*   } */
/*   /\* Mark as used and return. *\/ */
/*   ptr->available = 0; */
/*   return (ptr + 1); */
/* } */
