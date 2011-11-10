#ifndef __multiboot_parse_h__
#define __multiboot_parse_h__

/*
  File
  ----
  multiboot_parse.h
  
  Description
  -----------
  Functions for parsing multiboot data structures.

  Authors:
  Justin R. Wilson
*/

#include "multiboot.hpp"
#include "placement_allocator.hpp"

void
multiboot_parse_memory_map (const multiboot_info_t* multiboot_info,
			    placement_allocator_t* placement_allocator);

#endif /* __multiboot_parse_h__ */
