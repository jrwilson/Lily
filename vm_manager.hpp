#ifndef __vm_manager_hpp__
#define __vm_manager_hpp__

/*
  File
  ----
  vm_manager.hpp
  
  Description
  -----------
  The virtual memory manager manages logical memory.

  Authors:
  Justin R. Wilson
*/

#include "vm_def.hpp"

/* Number of entries in a page table or directory. */
static const size_t PAGE_ENTRY_COUNT = PAGE_SIZE / sizeof (void*);

namespace paging_constants {

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

  page_table_entry ();
  page_table_entry (frame_t frame,
		    paging_constants::page_privilege_t privilege,
		    paging_constants::writable_t writable,
		    paging_constants::present_t present);
};

struct page_table {
  page_table_entry entry[PAGE_ENTRY_COUNT];

  void
  clear (void);
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

  page_directory_entry ();
  page_directory_entry (frame_t frame,
			paging_constants::present_t present);
};

struct page_directory {
  page_directory_entry entry[PAGE_ENTRY_COUNT];

  void
  initialize ();

};

namespace vm_manager {
  void
  initialize ();

  physical_address_t
  page_directory_physical_address (void);

  page_directory*
  get_kernel_page_directory (void);

  void*
  get_stub (void);

  page_directory*
  get_page_directory (void);
  
  page_table*
  get_page_table (const void* address);

  void
  map (const void* address,
       frame_t frame,
       paging_constants::page_privilege_t privilege,
       paging_constants::writable_t writable);
  
  void
  unmap (const void* logical_addr);

  void
  switch_to_directory (physical_address_t physical_address);
};

#endif /* __vm_manager_hpp__ */
