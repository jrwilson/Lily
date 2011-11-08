/*
  File
  ----
  multiboot_preparse.c
  
  Description
  -----------
  Functions for "pre-parsing" multiboot data structures.
  Pre-parsing determines the lower and upper bounds of the multiboot data structures in memory.

  Authors
  -------
  Justin R. Wilson
*/

#include "multiboot_preparse.h"
#include "kassert.h"

static void
multiboot_new_begin (unsigned int* multiboot_begin,
		     unsigned int new_begin)
{
  *multiboot_begin = (new_begin < *multiboot_begin) ? new_begin : *multiboot_begin;
}

static void
multiboot_new_end (unsigned int* multiboot_end,
		   unsigned int new_end)
{
  *multiboot_end = (new_end > *multiboot_end) ? new_end : *multiboot_end;
}

int
multiboot_preparse_memory_map (const multiboot_info_t* multiboot_info,
			       unsigned int* multiboot_begin,
			       unsigned int* multiboot_end)
{
  kassert (multiboot_info != 0);
  kassert (multiboot_begin != 0);
  kassert (multiboot_end != 0);

  if (multiboot_info->flags & MULTIBOOT_INFO_MEM_MAP) {
    multiboot_new_begin (multiboot_begin, multiboot_info->mmap_addr);
    multiboot_new_end (multiboot_end, multiboot_info->mmap_addr + multiboot_info->mmap_length);
    return 0;
  }
  else {
    return -1;
  }
}
