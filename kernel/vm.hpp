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
#include "memory.hpp"

namespace vm {
  struct page_directory;
  struct page_table;
}

extern vm::page_directory kernel_page_directory;
extern vm::page_table kernel_page_table;
// A frame containing nothing but zeroes.
extern int zero_page;

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

  enum copy_on_write_t {
    NOT_COPY_ON_WRITE = 0,
    COPY_ON_WRITE = 1,
  };

  inline page_table_idx_t
  get_page_table_entry (logical_address_t address)
  {
    return (address & 0x3FF000) >> 12;
  }
  
  inline page_table_idx_t
  get_page_directory_entry (logical_address_t address)
  {
    return (address & 0xFFC00000) >> 22;
  }
  
  inline logical_address_t
  get_address (page_table_idx_t directory_entry,
	       page_table_idx_t table_entry)
  {
    return (directory_entry << 22 | table_entry << 12);
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
    unsigned int copy_on_write_ : 1;
    unsigned int available_ : 2;
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
      copy_on_write_ (NOT_COPY_ON_WRITE),
      available_ (0),
      frame_ (0)
    { }

    page_table_entry (frame_t frame,
		      copy_on_write_t cow,
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
      copy_on_write_ (cow),
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
    unsigned int copy_on_write_ : 1;
    unsigned int available_ : 2;
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
      copy_on_write_ (NOT_COPY_ON_WRITE),
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
      copy_on_write_ (NOT_COPY_ON_WRITE),
      available_ (0),
      frame_ (frame)
    { }
  };
  
  struct page_directory {
    page_directory_entry entry[PAGE_ENTRY_COUNT];

    page_directory (frame_t frame,
		    page_privilege_t paging_area_privilege,
		    page_privilege_t kernel_data_privilege)
    {
      // The kernel page directory.
      const page_directory* src = reinterpret_cast<page_directory*> (reinterpret_cast<logical_address_t> (&kernel_page_directory) + KERNEL_VIRTUAL_BASE);
      // Avoid the last entry.
      for (page_table_idx_t idx = get_page_directory_entry (KERNEL_VIRTUAL_BASE); idx != PAGE_ENTRY_COUNT - 1; ++idx) {
	if (src->entry[idx].present_ == PRESENT) {
	  entry[idx] = src->entry[idx];
	  frame_manager::incref (entry[idx].frame_);
	  entry[idx].user_ = kernel_data_privilege;
	}
      }

      // Map the page directory to itself.
      entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (frame, paging_area_privilege, PRESENT);
      frame_manager::incref (frame);
    }
  };

  inline physical_address_t
  get_kernel_page_directory_physical_address (void)
  {
    return reinterpret_cast<physical_address_t> (&kernel_page_directory);
  }

  inline page_directory*
  get_kernel_page_directory (void)
  {
    return reinterpret_cast<page_directory*> (reinterpret_cast<logical_address_t> (&kernel_page_directory) + KERNEL_VIRTUAL_BASE);
  }

  inline page_directory*
  get_page_directory (void)
  {
    /* Because the page directory is mapped to itself. */
    return reinterpret_cast<page_directory*> (0xFFFFF000);
  }

  inline page_table*
  get_page_table (logical_address_t address)
  {
    return reinterpret_cast<page_table*> (0xFFC00000 + get_page_directory_entry (address) * PAGE_SIZE);
  }
  
  inline frame_t
  page_directory_frame (void)
  {
    return get_page_directory ()->entry[PAGE_ENTRY_COUNT - 1].frame_;
  }

  inline logical_address_t
  get_stub1 (void)
  {
    // Reuse the address space of the page table.
    return reinterpret_cast<logical_address_t> (&kernel_page_table) + KERNEL_VIRTUAL_BASE;
  }

  // inline logical_address_t
  // get_stub2 (void)
  // {
  //   // Reuse the address space of the zero frame.
  //   return reinterpret_cast<logical_address_t> (&zero_page) + KERNEL_VIRTUAL_BASE;
  // }

  inline frame_t
  zero_frame ()
  {
    return physical_address_to_frame (reinterpret_cast<physical_address_t> (&zero_page));
  }

  inline frame_t
  logical_address_to_frame (logical_address_t logical_addr)
  {
    page_directory* pd = get_page_directory ();
    page_table* pt = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);

    kassert (pd->entry[directory_entry].present_ == PRESENT);
    kassert (pt->entry[table_entry].present_ == PRESENT);

    return pt->entry[table_entry].frame_;
  }

  enum map_mode_t {
    MAP_READ_WRITE = 0,
    MAP_READ_ONLY = 1,
    MAP_COPY_ON_WRITE = 2,
  };

  inline void
  map (logical_address_t logical_addr,
       frame_t fr,
       page_privilege_t privilege,
       map_mode_t map_mode,
       bool not_in_frame_manager)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* pt = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);

    if (page_directory->entry[directory_entry].present_ == NOT_PRESENT) {
      page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), USER, PRESENT);
      // Flush the TLB.
      asm ("invlpg (%0)\n" :: "r"(pt));
      // Initialize the page table.
      new (pt) vm::page_table ();
    }

    // The entry shoud not be present.
    kassert (pt->entry[table_entry].present_ == NOT_PRESENT);
    // Map.
    switch (map_mode) {
    case MAP_READ_WRITE:
      pt->entry[table_entry] = page_table_entry (fr, vm::NOT_COPY_ON_WRITE, privilege, vm::WRITABLE, PRESENT);
      break;
    case MAP_READ_ONLY:
      pt->entry[table_entry] = page_table_entry (fr, vm::NOT_COPY_ON_WRITE, privilege, vm::NOT_WRITABLE, PRESENT);
      break;
    case MAP_COPY_ON_WRITE:
      pt->entry[table_entry] = page_table_entry (fr, vm::COPY_ON_WRITE, privilege, vm::NOT_WRITABLE, PRESENT);
      break;
    }
    if (!not_in_frame_manager) {
      frame_manager::incref (fr);
    }
    // Flush the TLB.
    asm ("invlpg (%0)\n" :: "r"(logical_addr));
  }

  inline void
  remap (logical_address_t logical_addr,
	 page_privilege_t privilege,
	 map_mode_t map_mode)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);
    
    kassert (page_directory->entry[directory_entry].present_ == PRESENT);
    kassert (page_table->entry[table_entry].present_ == PRESENT);

    switch (map_mode) {
    case MAP_READ_WRITE:
      page_table->entry[table_entry] = page_table_entry (page_table->entry[table_entry].frame_, vm::NOT_COPY_ON_WRITE, privilege, vm::WRITABLE, PRESENT);
      break;
    case MAP_READ_ONLY:
      page_table->entry[table_entry] = page_table_entry (page_table->entry[table_entry].frame_, vm::NOT_COPY_ON_WRITE, privilege, vm::NOT_WRITABLE, PRESENT);
      break;
    case MAP_COPY_ON_WRITE:
      page_table->entry[table_entry] = page_table_entry (page_table->entry[table_entry].frame_, vm::COPY_ON_WRITE, privilege, vm::NOT_WRITABLE, PRESENT);
      break;
    }

    /* Flush the TLB. */
    asm ("invlpg (%0)\n" :: "r"(logical_addr));
  }

  inline void
  set_accessed (logical_address_t logical_addr,
		bool flag)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);
    
    kassert (page_directory->entry[directory_entry].present_ == PRESENT);

    page_table->entry[table_entry].accessed_ = flag;
    // No flushing required.
  }

  inline bool
  get_accessed (logical_address_t logical_addr)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);
    
    kassert (page_directory->entry[directory_entry].present_ == PRESENT);
    
    return page_table->entry[table_entry].accessed_;
  }

  inline copy_on_write_t
  get_copy_on_write (logical_address_t logical_addr)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);
    
    kassert (page_directory->entry[directory_entry].present_ == PRESENT);
    
    return static_cast<copy_on_write_t> (page_table->entry[table_entry].copy_on_write_);
  }

  inline void
  unmap (logical_address_t logical_addr,
	 bool decref = true)
  {
    page_directory* page_directory = get_page_directory ();
    page_table* page_table = get_page_table (logical_addr);
    const page_table_idx_t directory_entry = get_page_directory_entry (logical_addr);
    const page_table_idx_t table_entry = get_page_table_entry (logical_addr);
    
    kassert (page_directory->entry[directory_entry].present_ == PRESENT);
    kassert (page_table->entry[table_entry].present_ == PRESENT);

    if (decref) {
      frame_manager::decref (page_table->entry[table_entry].frame_);
    }
    page_table->entry[table_entry] = page_table_entry ();
    /* Flush the TLB. */
    asm ("invlpg (%0)\n" :: "r"(logical_addr));
  }

  inline physical_address_t
  switch_to_directory (physical_address_t n)
  {
    /* Switch to the page directory returning the old one. */
    physical_address_t old;
    asm ("mov %%cr3, %0\n" : "=r"(old));
    asm ("mov %0, %%cr3\n" :  : "r"(n));
    return old;
  }

  typedef uint32_t page_fault_error_t;
  
  inline bool
  not_present (page_fault_error_t error)
  {
    return (error & (1 << 0)) == 0;
  }
  
  inline bool
  protection_violation (page_fault_error_t error)
  {
    return (error & (1 << 0)) != 0;
  }
  
  inline bool
  read_context (page_fault_error_t error)
  {
    return (error & (1 << 1)) == 0;
  }
  
  inline bool
  write_context (page_fault_error_t error)
  {
    return (error & (1 << 1)) != 0;
  }
  
  inline bool
  supervisor_context (page_fault_error_t error)
  {
    return (error & (1 << 2)) == 0;
  }
  
  inline bool
  user_context (page_fault_error_t error)
  {
    return (error & (1 << 2)) != 0;
  }
  
  inline bool
  reserved_bit_violation (page_fault_error_t error)
  {
    return (error & (1 << 3)) != 0;
  }
  
  inline bool
  data_context (page_fault_error_t error)
  {
    return (error & (1 << 4)) == 0;
  }
  
  inline bool
  instruction_context (page_fault_error_t error)
  {
    return (error & (1 << 4)) != 0;
  }

};

#endif /* __vm_hpp__ */
