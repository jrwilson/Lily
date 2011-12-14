/*
  File
  ----
  vm_manager.c
  
  Description
  -----------
  The virtual memory manager manages logical memory.

  Authors:
  Justin R. Wilson
*/

#include "vm.hpp"
#include "system_automaton.hpp"
#include "automaton.hpp"

namespace vm {

  void
  initialize ()
  {
    kassert (system_automaton::system_automaton != 0);
    
    page_directory* kernel_page_directory = get_kernel_page_directory ();
    
    // Mark frames that are currently being used and update the logical address space.
    page_directory* pd = get_page_directory ();
    for (size_t i = 0; i < PAGE_ENTRY_COUNT; ++i) {
      if (pd->entry[i].present_) {
	page_table* pt = get_page_table (reinterpret_cast<const void*> (get_address (i, 0)));
	for (size_t j = 0; j < PAGE_ENTRY_COUNT; ++j) {
	  if (pt->entry[j].present_) {
	    const void* address = get_address (i, j);
	    // The page directory uses the same page table for low and high memory.  Only process high memory.
	    if (address >= KERNEL_VIRTUAL_BASE) {
	      if (system_automaton::system_automaton->address_in_use (address) || address == kernel_page_directory) {
		// If the address is in the logical address space, mark the frame as being used.
		frame_manager::mark_as_used (pt->entry[j].frame_);
	      }
	      else {
		// Otherwise, unmap it.
		pt->entry[j] = page_table_entry (0, SUPERVISOR, NOT_WRITABLE, NOT_PRESENT);
	      }
	    }
	  }
	}
      }
    }
    
    // I don't trust myself.
    // Consequently, I'm going to flush paging to ensure the subsequent memory accesses are correct.
    switch_to_directory (page_directory_frame ());
  }
}
