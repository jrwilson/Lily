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

#include "frame_manager.hpp"

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

  page_table_entry () :
    present_ (paging_constants::NOT_PRESENT),
    writable_ (paging_constants::NOT_WRITABLE),
    user_ (paging_constants::SUPERVISOR),
    write_through_ (paging_constants::WRITE_BACK),
    cache_disabled_ (paging_constants::CACHED),
    accessed_ (0),
    dirty_ (0),
    zero_ (0),
    global_ (paging_constants::NOT_GLOBAL),  
    available_ (0),
    frame_ (0)
  { }

  page_table_entry (frame frame,
		    paging_constants::page_privilege_t privilege,
		    paging_constants::writable_t writable,
		    paging_constants::present_t present) :
    present_ (present),
    writable_ (writable),
    user_ (privilege),
    write_through_ (paging_constants::WRITE_BACK),
    cache_disabled_ (paging_constants::CACHED),
    accessed_ (0),
    dirty_ (0),
    zero_ (0),
    global_ (paging_constants::NOT_GLOBAL),  
    available_ (0),
    frame_ (frame.f_)
  { }
};


struct page_table {
  page_table_entry entry[PAGE_ENTRY_COUNT];

  void
  clear (void)
  {
    kassert (logical_address (this).is_aligned (PAGE_SIZE));
    
    unsigned int k;
    for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
      entry[k] = page_table_entry (frame (), paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
    }
  }

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
    present_ (paging_constants::NOT_PRESENT),
    writable_ (paging_constants::WRITABLE),
    user_ (paging_constants::SUPERVISOR),
    write_through_ (paging_constants::WRITE_BACK),
    cache_disabled_ (paging_constants::CACHED),
    accessed_ (0),
    zero_ (0),
    page_size_ (paging_constants::PAGE_SIZE_4K),
    ignored_ (0),
    available_ (0),
    frame_ (0)
  { }

  page_directory_entry (frame frame,
			paging_constants::present_t present) :
    present_ (present),
    writable_ (paging_constants::WRITABLE),
    user_ (paging_constants::SUPERVISOR),
    write_through_ (paging_constants::WRITE_BACK),
    cache_disabled_ (paging_constants::CACHED),
    accessed_ (0),
    zero_ (0),
    page_size_ (paging_constants::PAGE_SIZE_4K),
    ignored_ (0),
    available_ (0),
    frame_ (frame.f_)
  { }
};

struct page_directory {
  page_directory_entry entry[PAGE_ENTRY_COUNT];

  void
  clear (physical_address address)
  {
    kassert (logical_address (this).is_aligned (PAGE_SIZE));
    kassert (address.is_aligned (PAGE_SIZE));
    
    /* Clear the page directory and page table. */
    unsigned int k;
    for (k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
      entry[k] = page_directory_entry (frame (), paging_constants::NOT_PRESENT);
    }
    
    /* Map the page directory to itself. */
    entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (frame (address), paging_constants::PRESENT);
  }

};

class vm_manager {
private:
  page_directory kernel_page_directory __attribute__ ((aligned (PAGE_SIZE)));
  page_table low_page_table __attribute__ ((aligned (PAGE_SIZE)));

public:
  vm_manager (logical_address placement_begin,
	      logical_address placement_end,
	      frame_manager& fm);

  // logical_address
  // vm_manager_page_directory_logical_address (void);

  // physical_address
  // vm_manager_page_directory_physical_address (void);

  // void
  // vm_manager_map (logical_address address,
  // 		frame frame,
  // 		page_privilege_t privilege,
  // 		writable_t writable);

  // void
  // vm_manager_unmap (logical_address logical_addr);

  // void
  // vm_manager_switch_to_directory (physical_address address);

  // void
  // page_directory_initialize_with_current (page_directory_t* page_directory,
  // 					physical_address address);

};

#endif /* __vm_manager_hpp__ */
