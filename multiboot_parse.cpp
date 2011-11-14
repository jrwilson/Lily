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

#include "multiboot_parse.hpp"
#include "kassert.hpp"
#include "types.hpp"
#include "frame_manager.hpp"
#include "algorithm.hpp"

void
multiboot_parse_memory_map (const multiboot_info_t* multiboot_info,
			    placement_allocator& placement_allocator)
{
  kassert (multiboot_info != 0);

  /* Parse the memory map. */
  multiboot_memory_map_t* ptr = (multiboot_memory_map_t*)multiboot_info->mmap_addr;
  multiboot_memory_map_t* limit = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + multiboot_info->mmap_length);
  while (ptr < limit) {
    kputx64 (ptr->addr); kputs ("-"); kputx64 (ptr->addr + ptr->len - 1);
    switch (ptr->type) {
    case MULTIBOOT_MEMORY_AVAILABLE:
      kputs (" AVAILABLE\n");
      {
	physical_address begin (max (static_cast<multiboot_uint64_t> (USABLE_MEMORY_BEGIN.value ()), ptr->addr));
	physical_address end (min (static_cast<multiboot_uint64_t> (USABLE_MEMORY_END.value ()), ptr->addr + ptr->len));
	frame_manager::add (placement_allocator, begin, end);
      }
      break;
    case MULTIBOOT_MEMORY_RESERVED:
      kputs (" RESERVED\n");
      break;
    default:
      kputs (" UNKNOWN\n");
      break;
    }
    /* Move to the next descriptor. */
    ptr = (multiboot_memory_map_t*) (((char*)&(ptr->addr)) + ptr->size);
  }
}
