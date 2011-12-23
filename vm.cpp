#include "vm.hpp"
#include "rts.hpp"

namespace vm {
  
  inline logical_address_t
  get_stub2 (void)
  {
    // Reuse the address space of the zero frame.
    return reinterpret_cast<logical_address_t> (&zero_page) + KERNEL_VIRTUAL_BASE;
  }

  void
  expand_kernel (page_table_idx_t directory_entry)
  {
    // Allocate a new page table.
    page_directory_entry e (frame_manager::alloc (), USER, PRESENT);
    
    // Map in the kernel directory.
    get_kernel_page_directory ()->entry[directory_entry] = e;
    
    // Map in all the other directories.
    for (rts::const_automaton_iterator pos = rts::automaton_begin ();
         pos != rts::automaton_end ();
         ++pos) {
      map (get_stub2 (), pos->page_directory_frame (), USER, WRITABLE);
      reinterpret_cast<page_directory*> (get_stub2 ())->entry[directory_entry] = e;
      frame_manager::incref (e.frame_);
      unmap (get_stub2 ());
    }
  }

}
