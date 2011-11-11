#ifndef __vm_manager_h__
#define __vm_manager_h__

/*
  File
  ----
  vm_manager.h
  
  Description
  -----------
  The virtual memory manager manages logical memory.

  Authors:
  Justin R. Wilson
*/

#include "descriptor.hpp"
#include "frame_manager.hpp"

#define PAGE_SIZE 0x1000

/* Should agree with loader.s. */
const logical_address KERNEL_VIRTUAL_BASE (reinterpret_cast<void*> (0xC0000000));

/* Number of entries in a page table or directory. */
#define PAGE_ENTRY_COUNT 1024

typedef enum {
  NOT_GLOBAL = 0,
  GLOBAL = 1,
} global_t;

typedef enum {
  CACHED = 0,
  NOT_CACHED = 1,
} cached_t;

typedef enum {
  WRITE_BACK = 0,
  WRITE_THROUGH = 1,
} cache_mode_t;

typedef enum {
  SUPERVISOR = 0,
  USER = 1,
} page_privilege_t;

typedef enum {
  PAGE_SIZE_4K = 0,
  PAGE_SIZE_4M = 1,
} page_size_t;

struct page_table_entry_t {
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

  page_table_entry_t () :
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

  page_table_entry_t (frame frame,
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
    frame_ (frame.f_)
  { }
};


typedef struct {
  page_table_entry_t entry[PAGE_ENTRY_COUNT];
} page_table_t;

struct page_directory_entry_t {
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

  page_directory_entry_t () :
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

  page_directory_entry_t (frame frame,
			  present_t present) :
    present_ (present),
    writable_ (WRITABLE),
    user_ (SUPERVISOR),
    write_through_ (WRITE_BACK),
    cache_disabled_ (CACHED),
    accessed_ (0),
    zero_ (0),
    page_size_ (PAGE_SIZE_4K),
    ignored_ (0),
    available_ (0),
    frame_ (frame.f_)
  { }
};

typedef struct {
  page_directory_entry_t entry[PAGE_ENTRY_COUNT];
} page_directory_t;

void
vm_manager_initialize (logical_address placement_begin,
		       logical_address placement_end);

logical_address
vm_manager_page_directory_logical_address (void);

physical_address
vm_manager_page_directory_physical_address (void);

void
vm_manager_map (logical_address address,
		frame frame,
		page_privilege_t privilege,
		writable_t writable);

void
vm_manager_unmap (logical_address logical_addr);

void
vm_manager_switch_to_directory (physical_address address);

void
page_directory_initialize_with_current (page_directory_t* page_directory,
					physical_address address);

#endif /* __vm_manager_h__ */
