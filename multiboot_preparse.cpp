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

#include "multiboot_preparse.hpp"
#include "kassert.hpp"
#include "algorithm.hpp"

static void
multiboot_new_begin (physical_address& multiboot_begin,
		     size_t new_begin)
{
  multiboot_begin = min (multiboot_begin, physical_address (new_begin));
}

static void
multiboot_new_end (physical_address& multiboot_end,
		   size_t new_end)
{
  multiboot_end = max (multiboot_end, physical_address (new_end));
}

int
multiboot_preparse_memory_map (const multiboot_info_t* multiboot_info,
			       physical_address& multiboot_begin,
			       physical_address& multiboot_end)
{
  kassert (multiboot_info != 0);

  if (multiboot_info->flags & MULTIBOOT_INFO_MEM_MAP) {
    multiboot_new_begin (multiboot_begin, multiboot_info->mmap_addr);
    multiboot_new_end (multiboot_end, multiboot_info->mmap_addr + multiboot_info->mmap_length);
    return 0;
  }
  else {
    return -1;
  }
}
