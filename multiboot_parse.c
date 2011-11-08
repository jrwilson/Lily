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

#include "multiboot_parse.h"
#include "kassert.h"
#include "types.h"
#include "frame_manager.h"

void
multiboot_parse_memory_map (const multiboot_info_t* multiboot_info,
			    placement_allocator_t* placement_allocator)
{
  kassert (multiboot_info != 0);

  /* Parse the memory map. */
  multiboot_memory_map_t* ptr = (multiboot_memory_map_t*)multiboot_info->mmap_addr;
  multiboot_memory_map_t* limit = (multiboot_memory_map_t*)(multiboot_info->mmap_addr + multiboot_info->mmap_length);
  while (ptr < limit) {
    uint64_t first = ptr->addr;
    uint64_t last = ptr->addr + ptr->len - 1;
    kputx64 (first); kputs ("-"); kputx64 (last);
    switch (ptr->type) {
    case MULTIBOOT_MEMORY_AVAILABLE:
      kputs (" AVAILABLE\n");
      frame_manager_add (placement_allocator, first, last);
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
