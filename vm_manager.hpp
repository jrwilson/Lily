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
#include "frame.hpp"

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
  page_table_entry (frame frame,
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
  page_directory_entry (frame frame,
			paging_constants::present_t present);
};

struct page_directory {
  page_directory_entry entry[PAGE_ENTRY_COUNT];

  void
  clear (physical_address address);

  void
  initialize_with_current (physical_address address);

};

class vm_manager {
private:
  static page_directory kernel_page_directory __attribute__ ((aligned (PAGE_SIZE)));
  static page_table low_page_table __attribute__ ((aligned (PAGE_SIZE)));

  friend class page_directory;

  static page_directory*
  get_page_directory (void);
  
  static page_table*
  get_page_table (logical_address address);

  vm_manager ();
  vm_manager (const vm_manager&);
  vm_manager& operator= (const vm_manager&);

public:
  static void
  initialize (logical_address placement_begin,
	      logical_address placement_end);

  static physical_address
  page_directory_physical_address (void);
  
  static logical_address
  page_directory_logical_address (void);

  static void
  map (logical_address address,
       frame frame,
       paging_constants::page_privilege_t privilege,
       paging_constants::writable_t writable);
  
  static void
  unmap (logical_address logical_addr);

  static void
  switch_to_directory (physical_address address);
};

#endif /* __vm_manager_hpp__ */
