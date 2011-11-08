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

#include "descriptor.h"

#define PAGE_SIZE 0x1000

/* Should agree with loader.s. */
#define KERNEL_VIRTUAL_BASE 0xC0000000

#define PAGE_ALIGN_DOWN(addr) ((unsigned int)(addr) & 0xFFFFF000)
#define PAGE_ALIGN_UP(addr) (PAGE_ALIGN_DOWN(((unsigned int)(addr) + PAGE_SIZE - 1)))
#define IS_PAGE_ALIGNED(addr) (((addr) & 0xFFF) == 0)

/* Convert physical addresses to frame numbers and vice versa. */
#define ADDRESS_TO_FRAME(addr) ((addr) >> 12)
#define FRAME_TO_ADDRESS(addr) ((addr) << 12)

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

typedef struct {
  unsigned int present : 1;
  unsigned int writable : 1;
  unsigned int user : 1;
  unsigned int write_through : 1;
  unsigned int cache_disabled : 1;
  unsigned int accessed : 1;
  unsigned int dirty : 1;
  unsigned int zero : 1;
  unsigned int global : 1;
  unsigned int available : 3;
  unsigned int frame : 20;
} page_table_entry_t;

typedef struct {
  unsigned int present : 1;
  unsigned int writable : 1;
  unsigned int user : 1;
  unsigned int write_through : 1;
  unsigned int cache_disabled : 1;
  unsigned int accessed : 1;
  unsigned int zero : 1;
  unsigned int page_size : 1;
  unsigned int ignored : 1;
  unsigned int available : 3;
  unsigned int frame : 20;
} page_directory_entry_t;

void
vm_manager_initialize (void* placement_begin,
		       void* placement_end);

void*
vm_manager_page_directory_logical_address (void);

unsigned int
vm_manager_page_directory_physical_address (void);

void
vm_manager_map (void* logical_addr,
		unsigned int frame,
		page_privilege_t privilege,
		writable_t writable,
		present_t present);

#endif /* __vm_manager_h__ */
