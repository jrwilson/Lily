#ifndef __multiboot_preparse_h__
#define __multiboot_preparse_h__

/*
  File
  ----
  multiboot_preparse.h
  
  Description
  -----------
  Functions for "pre-parsing" multiboot data structures.
  Pre-parsing determines the lower and upper bounds of the multiboot data structures in memory.

  Authors:
  Justin R. Wilson
*/

#include "multiboot.hpp"
#include "types.hpp"

int
multiboot_preparse_memory_map (const multiboot_info_t* multiboot_info,
			       physical_address& multiboot_begin,
			       physical_address& multiboot_end);

#endif /* __multiboot_preparse_h__ */
