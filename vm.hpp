#ifndef __vm_hpp__
#define __vm_hpp__

/*
  File
  ----
  vm.hpp
  
  Description
  -----------
  The virtual memory manager manages logical memory.

  Authors:
  Justin R. Wilson
*/

#include "vm_def.hpp"
#include "kassert.hpp"
#include "frame_manager.hpp"

namespace vm {
  struct page_directory;
  struct page_table;
}

extern vm::page_directory kernel_page_directory;
extern vm::page_table kernel_page_table;

class automaton;

namespace vm {

  enum global_t {
    NOT_GLOBAL = 0,
    GLOBAL = 1,
  };

  enum cached_t {
    CACHED = 0,
    NOT_CACHED = 1,
  };

  enum cache_mode_t {
    WRITE_BACK = 0,
    WRITE_THROUGH = 1,
  };

  enum page_privilege_t {
    SUPERVISOR = 0,
    USER = 1,
  };

  enum page_size_t {
    PAGE_SIZE_4K = 0,
    PAGE_SIZE_4M = 1,
  };

  enum writable_t {
    NOT_WRITABLE = 0,
    WRITABLE = 1,
  };

  enum present_t {
    NOT_PRESENT = 0,
    PRESENT = 1,
  };

  inline page_table_idx_t
  get_page_table_entry (const void* address)
  {
    return (reinterpret_cast<uintintptr_t> (address) & 0x3FF000) >> 12;
  }
  
  inline page_table_idx_t
  get_page_directory_entry (const void* address)
  {
    return (reinterpret_cast<uintintptr_t> (address) & 0xFFC00000) >> 22;
  }
  
  inline const void*
  get_address (page_table_idx_t directory_entry,
	       page_table_idx_t table_entry)
  {
    return reinterpret_cast<const void*> (directory_entry << 22 | table_entry << 12);
  }

  struct page_table_entry {
    unsigned int present_ : 1;
    unsigned int writable_ : 1;
    unsigned int user_ : 1;
    unsigned int write_through_ : 1;
    unsigned int cache_disabled_ : 1;
    unsigned int accessed_ : 1;
    unsigned int dirty_ : 1;
    unsigned int zero_ : 1;
    unsigned int global_ : 1;
    unsigned int available_ : 3;
    unsigned int frame_ : 20;
    
    page_table_entry () :
      present_ (NOT_PRESENT),
      writable_ (NOT_WRITABLE),
      user_ (SUPERVISOR),
      write_through_ (WRITE_BACK),
      cache_disabled_ (CACHED),
      accessed_ (0),
      dirty_ (0),
      zero_ (0),
      global_ (NOT_GLOBAL),  
      available_ (0),
      frame_ (0)
    { }

    page_table_entry (frame_t frame,
		      page_privilege_t privilege,
		      writable_t writable,
		      present_t present) :
      present_ (present),
      writable_ (writable),
      user_ (privilege),
      write_through_ (WRITE_BACK),
      cache_disabled_ (CACHED),
      accessed_ (0),
      dirty_ (0),
      zero_ (0),
      global_ (NOT_GLOBAL),  
      available_ (0),
      frame_ (frame)
    { }
  };
  
  struct page_table {
    page_table_entry entry[PAGE_ENTRY_COUNT];
  };
  
  struct page_directory_entry {
    unsigned int present_ : 1;
    unsigned int writable_ : 1;
    unsigned int user_ : 1;
    unsigned int write_through_ : 1;
    unsigned int cache_disabled_ : 1;
    unsigned int accessed_ : 1;
    unsigned int zero_ : 1;
    unsigned int page_size_ : 1;
    unsigned int ignored_ : 1;
    unsigned int available_ : 3;
    unsigned int frame_ : 20;

    page_directory_entry () :
      present_ (NOT_PRESENT),
      writable_ (WRITABLE),
      user_ (SUPERVISOR),
      write_through_ (WRITE_BACK),
      cache_disabled_ (CACHED),
      accessed_ (0),
      zero_ (0),
      page_size_ (PAGE_SIZE_4K),
      ignored_ (0),
      available_ (0),
      frame_ (0)
    { }
    
    page_directory_entry (frame_t frame,
			  page_privilege_t privilege,
			  present_t present) :
      present_ (present),
      writable_ (WRITABLE),
      user_ (privilege),
      write_through_ (WRITE_BACK),
      cache_disabled_ (CACHED),
      accessed_ (0),
      zero_ (0),
      page_size_ (PAGE_SIZE_4K),
      ignored_ (0),
      available_ (0),
      frame_ (frame)
    { }
  };
  
  struct page_directory;

  inline page_directory*
  get_kernel_page_directory (void)
  {
    return reinterpret_cast<page_directory*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (&kernel_page_directory));
  };

  inline page_table*
  get_page_table (const void* address)
  {
    return reinterpret_cast<page_table*> (0xFFC00000 + get_page_directory_entry (address) * PAGE_SIZE);
  }

  struct page_directory {
    page_directory_entry entry[PAGE_ENTRY_COUNT];

    page_directory (bool all)
    {
      kassert (is_aligned (this, PAGE_SIZE));
      
      // Copy the kernel page directory.
      page_directory* kernel_directory = get_kernel_page_directory ();
      for (size_t k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
	if (all || get_address (k, 0) >= KERNEL_VIRTUAL_BASE) {
	  entry[k] = kernel_directory->entry[k];
	  if (entry[k].present_) {
	    frame_manager::incref (entry[k].frame_);
	  }
	}
	else {
	  entry[k] = page_directory_entry (0, SUPERVISOR, NOT_PRESENT);
	}
      }
      
      // Find the frame corresponding to this page directory.
      page_table* page_table = get_page_table (this);
      const size_t table_entry = get_page_table_entry (this);
      frame_t frame = page_table->entry[table_entry].frame_;
      
      // Map the page directory to itself.
      entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (frame, SUPERVISOR, PRESENT);
      frame_manager::incref (frame);
    }
  };
  
  void
  initialize (automaton*);

  inline page_directory*
  get_page_directory (void)
  {
    /* Because the page directory is mapped to itself. */
    return reinterpret_cast<page_directory*> (0xFFFFF000);
  }
  
  inline frame_t
  page_directory_frame (void)
  {
    return get_page_directory ()->entry[PAGE_ENTRY_COUNT - 1].frame_;
  }
    
  inline void*
  get_stub (void)
  {
    // Reuse the address space of the page table.
    return reinterpret_cast<void*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (&kernel_page_table));
  }

  inline void
  map (const void* logical_addr,
       frame_t fr,
       page_privilege_t privilege,
       writable_t writable)
  {
    page_directory* kernel_page_directory = get_kernel_page_directory ();
    page_directory* page_directory = get_page_directory ();
    page_table* pt = get_page_table (logical_addr);
    const size_t directory_entry = get_page_directory_entry (logical_addr);
    const size_t table_entry = get_page_table_entry (logical_addr);
    
    // Find or create a page table.
    if (!page_directory->entry[directory_entry].present_) {
      if (logical_addr < KERNEL_VIRTUAL_BASE ||
	  page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_ == kernel_page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_) {
	// The address is in user space or we are using the kernel page directory.
	// Either way, we can just allocate a page table.
	page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), USER, PRESENT);
	// Flush the TLB.
	asm ("invlpg (%0)\n" :: "r"(pt));
	// Initialize the page table.
	new (pt) vm::page_table ();
      }
      else {
	// We are using a non-kernel page directory and need a kernel page table.
	if (!kernel_page_directory->entry[directory_entry].present_) {
	  // The page table is not present in the kernel.
	  // Allocate a page table and map it in both directories.
	  kernel_page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), USER, PRESENT);
	  page_directory->entry[directory_entry] = kernel_page_directory->entry[directory_entry];
	  frame_manager::incref (page_directory->entry[directory_entry].frame_);
	  // Flush the TLB.
	  asm ("invlpg (%0)\n" :: "r"(pt));
	  // Initialize the page table.
	  new (pt) vm::page_table ();
	}
	else {
	  // The page table is present in the kernel.
	  // Copy the entry.
	  page_directory->entry[directory_entry] = kernel_page_directory->entry[directory_entry];
	  frame_manager::incref (page_directory->entry[directory_entry].frame_);
	  // Flush the TLB.
	  asm ("invlpg (%0)\n" :: "r"(pt));
	}
      }
    }
    
    kassert (pt->entry[table_entry].present_ == NOT_PRESENT);
    pt->entry[table_entry] = page_table_entry (fr, privilege, writable, PRESENT);
    frame_manager::incref (fr);

    // Flush the TLB.
    asm ("invlpg (%0)\n" :: "r"(logical_addr));
  }

  inline void
  remap (const void* logical_addr,
	 page_privilege_t privilege,
	 writable_t writable)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const size_t directory_entry = get_page_directory_entry (logical_addr);
    const size_t table_entry = get_page_table_entry (logical_addr);
    
    kassert (page_directory->entry[directory_entry].present_ == PRESENT);
    kassert (page_table->entry[table_entry].present_ == PRESENT);

    page_table->entry[table_entry] = page_table_entry (page_table->entry[table_entry].frame_, privilege, writable, PRESENT);
    /* Flush the TLB. */
    asm ("invlpg (%0)\n" :: "r" (logical_addr));
  }
  
  inline bool
  unmap (const void* logical_addr)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const size_t directory_entry = get_page_directory_entry (logical_addr);
    const size_t table_entry = get_page_table_entry (logical_addr);
    
    if (page_directory->entry[directory_entry].present_ == PRESENT &&
	page_table->entry[table_entry].present_ == PRESENT) {
      frame_manager::decref (page_table->entry[table_entry].frame_);
      page_table->entry[table_entry] = page_table_entry (0, SUPERVISOR, NOT_WRITABLE, NOT_PRESENT);
      /* Flush the TLB. */
      asm ("invlpg (%0)\n" :: "r"(logical_addr));
      return true;
    }
    else {
      return false;
    }
  }

  inline void
  switch_to_directory (frame_t frame)
  {
    /* Switch to the page directory. */
    asm ("mov %0, %%cr3\n" :: "g"(frame_to_physical_address (frame)));
  }
};

#endif /* __vm_hpp__ */
