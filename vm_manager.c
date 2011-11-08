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

#include "vm_manager.h"
#include "kassert.h"
#include "frame_manager.h"
#include "idt.h"

/* Everything must fit into 4MB initialy. */
#define INITIAL_LOGICAL_LIMIT (KERNEL_VIRTUAL_BASE + 0x400000)

/* Number of entries in a page table or directory. */
#define PAGE_ENTRY_COUNT 1024

/* Macros for address manipulation. */
#define PAGE_DIRECTORY_ENTRY(addr)  (((size_t)(addr) & 0xFFC00000) >> 22)
#define PAGE_TABLE_ENTRY(addr) (((size_t)(addr) & 0x3FF000) >> 12)
#define PAGE_ADDRESS_OFFSET(addr) ((addr) & 0xFFF)

typedef struct {
  page_table_entry_t entry[PAGE_ENTRY_COUNT];
} page_table_t;

typedef struct {
  page_directory_entry_t entry[PAGE_ENTRY_COUNT];
} page_directory_t;

/* Marks the beginning and end of the kernel upon loading. */
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;

static page_directory_t kernel_page_directory __attribute__ ((aligned (PAGE_SIZE)));
static page_table_t low_page_table __attribute__ ((aligned (PAGE_SIZE)));

static page_table_entry_t
make_page_table_entry (frame_t frame,
		       page_privilege_t privilege,
		       writable_t writable,
		       present_t present)
{
  page_table_entry_t retval;
  retval.frame = frame;
  retval.available = 0;
  retval.global = NOT_GLOBAL;
  retval.zero = 0;
  retval.dirty = 0;
  retval.accessed = 0;
  retval.cache_disabled = CACHED;
  retval.write_through = WRITE_BACK;
  retval.user = privilege;
  retval.writable = writable;
  retval.present = present;
  return retval;
}

static page_directory_entry_t
make_page_directory_entry (frame_t frame,
			   present_t present)
{
  page_directory_entry_t retval;
  retval.frame = frame;
  retval.available = 0;
  retval.ignored = 0;
  retval.page_size = PAGE_SIZE_4K;
  retval.zero = 0;
  retval.accessed = 0;
  retval.cache_disabled = CACHED;
  retval.write_through = WRITE_BACK;
  retval.user = SUPERVISOR;
  retval.writable = WRITABLE;
  retval.present = present;
  return retval;
}

static void
initialize_page_table (page_table_t* ptr)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((size_t)ptr));

  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    ptr->entry[k] = make_page_table_entry (0, 0, 0, 0);
  }
}

static void
initialize_page_directory (page_directory_t* ptr,
			   size_t physical_address)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((size_t)ptr));
  kassert (IS_PAGE_ALIGNED ((size_t)physical_address));

  /* Clear the page directory and page table. */
  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    ptr->entry[k] = make_page_directory_entry (0, NOT_PRESENT);
  }

  /* Map the page directory to itself. */
  ptr->entry[PAGE_ENTRY_COUNT - 1] = make_page_directory_entry (ADDRESS_TO_FRAME (physical_address), PRESENT);
}

void
vm_manager_switch_to_directory (size_t physical_addr)
{
  kassert (IS_PAGE_ALIGNED (physical_addr));

  /* Switch to the page directory. */
  asm volatile ("mov %0, %%eax\n"
  		"mov %%eax, %%cr3\n" :: "r"(physical_addr) : "%eax");
}

void
vm_manager_initialize (void* placement_begin,
		       void* placement_end)
{
  size_t low_page_table_paddr = (size_t)&low_page_table - KERNEL_VIRTUAL_BASE;
  size_t kernel_page_directory_paddr = (size_t)&kernel_page_directory - KERNEL_VIRTUAL_BASE;

  /* Clear the page directory and page table. */
  initialize_page_table (&low_page_table);
  initialize_page_directory (&kernel_page_directory, kernel_page_directory_paddr);

  /* Insert the page table. */
  kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY(0)] = make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), PRESENT);
  kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY (KERNEL_VIRTUAL_BASE)] = make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), PRESENT);

  void* begin;
  void* end;

  /* Identity map the first 1MB. */
  begin = 0;
  end = (void*)0x100000;
  kassert (end < (void*)INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    frame_t frame = ADDRESS_TO_FRAME ((size_t)begin);
    low_page_table.entry[PAGE_TABLE_ENTRY (begin)] = make_page_table_entry (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }

  /* Map the kernel text. */
  begin = (void*)PAGE_ALIGN_DOWN ((size_t)&text_begin);
  end = (void*)PAGE_ALIGN_UP ((size_t)&text_end);
  kassert (end < (void*)INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    frame_t frame = ADDRESS_TO_FRAME ((size_t)begin - KERNEL_VIRTUAL_BASE);
    low_page_table.entry[PAGE_TABLE_ENTRY (begin)] = make_page_table_entry (frame, SUPERVISOR, NOT_WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }

  /* Map the kernel data. */
  begin = (void*)PAGE_ALIGN_DOWN ((size_t)&data_begin);
  end = (void*)PAGE_ALIGN_UP ((size_t)&data_end);
  kassert (end < (void*)INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    frame_t frame = ADDRESS_TO_FRAME ((size_t)begin - KERNEL_VIRTUAL_BASE);
    low_page_table.entry[PAGE_TABLE_ENTRY (begin)] = make_page_table_entry (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }

  /* Map the memory allocated by placement. */
  begin = (void*)PAGE_ALIGN_DOWN ((size_t)placement_begin);
  end = (void*)PAGE_ALIGN_UP ((size_t)placement_end);
  kassert (end < (void*)INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    frame_t frame = ADDRESS_TO_FRAME ((size_t)begin - KERNEL_VIRTUAL_BASE);
    low_page_table.entry[PAGE_TABLE_ENTRY (begin)] = make_page_table_entry (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }

  vm_manager_switch_to_directory (kernel_page_directory_paddr);
}

page_directory_t*
get_page_directory (void)
{
  /* Because the page directory is mapped to itself. */
  return (page_directory_t*)0xFFFFF000;
}

static page_table_t*
get_page_table (void* logical_addr)
{
  return (page_table_t*)(0xFFC00000 + PAGE_DIRECTORY_ENTRY (logical_addr) * PAGE_SIZE);
}

unsigned int
vm_manager_page_directory_physical_address (void)
{
  page_directory_t* page_directory = get_page_directory ();
  return FRAME_TO_ADDRESS (page_directory->entry[PAGE_ENTRY_COUNT - 1].frame);
}

void*
vm_manager_page_directory_logical_address (void)
{
  return get_page_directory ();
}

void
vm_manager_map (void* logical_addr,
		frame_t frame,
		page_privilege_t privilege,
		writable_t writable,
		present_t present)
{
  page_directory_t* page_directory = get_page_directory ();
  page_table_t* page_table = get_page_table (logical_addr);

  if (!page_directory->entry[PAGE_DIRECTORY_ENTRY(logical_addr)].present) {
    if (logical_addr < (void*)KERNEL_VIRTUAL_BASE ||
	page_directory->entry[PAGE_ENTRY_COUNT - 1].frame == kernel_page_directory.entry[PAGE_ENTRY_COUNT - 1].frame) {
      /* Either the address is in user space or we are using the kernel page directory. */
      /* Allocate a page table. */
      page_directory->entry[PAGE_DIRECTORY_ENTRY (logical_addr)] = make_page_directory_entry (frame_manager_alloc (), PRESENT);
      /* Flush the TLB. */
      asm volatile ("invlpg %0\n" :: "m" (page_table));
      /* Initialize the page table. */
      initialize_page_table (page_table);
    }
    else {
      /* Using a non-kernel page directory but we need a kernel page table. */
      if (!kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY(logical_addr)].present) {
	/* The page table is not present in the kernel. */
	/* Allocate a page table and map it in both directories. */
	kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY (logical_addr)] = make_page_directory_entry (frame_manager_alloc (), PRESENT);
	page_directory->entry[PAGE_DIRECTORY_ENTRY (logical_addr)] = kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY(logical_addr)];
	frame_manager_incref (page_directory->entry[PAGE_DIRECTORY_ENTRY (logical_addr)].frame);
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
	/* Initialize the page table. */
	initialize_page_table (page_table);
      }
      else {
	/* The page table is present in the kernel. */
	/* Copy the entry. */
	page_directory->entry[PAGE_DIRECTORY_ENTRY (logical_addr)] = kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY(logical_addr)];
	frame_manager_incref (page_directory->entry[PAGE_DIRECTORY_ENTRY (logical_addr)].frame);
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
      }
    }
  }

  page_table->entry[PAGE_TABLE_ENTRY (logical_addr)] = make_page_table_entry (frame, privilege, writable, present);

  /* Flush the TLB. */
  asm volatile ("invlpg %0\n" :: "m" (logical_addr));
}
