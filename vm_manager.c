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

/* Number of entries in a page table or directory. */
#define PAGE_ENTRY_COUNT 1024

/* Macros for address manipulation. */
#define PAGE_DIRECTORY_ENTRY(addr)  (((unsigned int)(addr) & 0xFFC00000) >> 22)
#define PAGE_TABLE_ENTRY(addr) (((unsigned int)(addr) & 0x3FF000) >> 12)
#define PAGE_ADDRESS_OFFSET(addr) ((addr) & 0xFFF)

typedef struct {
  page_table_entry_t entry[PAGE_ENTRY_COUNT];
} page_table_t;

typedef struct {
  page_directory_entry_t entry[PAGE_ENTRY_COUNT];
} page_directory_t;

/* Marks the beginning and end of the kernel upon loading. */
extern int kernel_begin;
extern int kernel_end;

static page_directory_t kernel_page_directory __attribute__ ((aligned (PAGE_SIZE)));
static page_table_t low_page_table __attribute__ ((aligned (PAGE_SIZE)));

static page_table_entry_t
make_page_table_entry (unsigned int frame,
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
make_page_directory_entry (unsigned int frame,
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
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));

  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    ptr->entry[k] = make_page_table_entry (0, 0, 0, 0);
  }
}

static void
initialize_page_directory (page_directory_t* ptr,
			   unsigned int physical_address)
{
  kassert (ptr != 0);
  kassert (IS_PAGE_ALIGNED ((unsigned int)ptr));
  kassert (IS_PAGE_ALIGNED ((unsigned int)physical_address));

  /* Clear the page directory and page table. */
  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    ptr->entry[k] = make_page_directory_entry (0, NOT_PRESENT);
  }

  /* Map the page directory to itself. */
  ptr->entry[PAGE_ENTRY_COUNT - 1] = make_page_directory_entry (ADDRESS_TO_FRAME (physical_address), PRESENT);
}

static void
switch_to_page_directory (unsigned int physical_addr)
{
  kassert (IS_PAGE_ALIGNED (physical_addr));

  /* Switch to the page directory. */
  asm volatile ("mov %0, %%eax\n"
  		"mov %%eax, %%cr3\n" :: "r"(physical_addr) : "%eax");
}

void
vm_manager_initialize (void)
{
  unsigned int low_page_table_paddr = (unsigned int)&low_page_table - KERNEL_VIRTUAL_BASE;
  unsigned int kernel_page_directory_paddr = (unsigned int)&kernel_page_directory - KERNEL_VIRTUAL_BASE;

  /* Clear the page directory and page table. */
  initialize_page_table (&low_page_table);
  initialize_page_directory (&kernel_page_directory, kernel_page_directory_paddr);

  /* Insert the page table. */
  kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY(0)] = make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), PRESENT);
  kernel_page_directory.entry[PAGE_DIRECTORY_ENTRY (KERNEL_VIRTUAL_BASE)] = make_page_directory_entry (ADDRESS_TO_FRAME (low_page_table_paddr), PRESENT);

  /* Identity map the first 1M. */
  unsigned int physical_addr;
  for (physical_addr = 0; physical_addr < 0x100000; physical_addr += PAGE_SIZE) {
    /* Do this manually. */
    unsigned int frame = ADDRESS_TO_FRAME (physical_addr);
    low_page_table.entry[PAGE_TABLE_ENTRY (physical_addr)] = make_page_table_entry (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }

  /* Map the kernel. */
  unsigned int begin = PAGE_ALIGN_DOWN ((unsigned int)&kernel_begin - KERNEL_VIRTUAL_BASE);
  unsigned int end = PAGE_ALIGN_UP ((unsigned int)&kernel_end - KERNEL_VIRTUAL_BASE);
  /* The frame manager checked that these are under 4M. */
  for (physical_addr = begin; physical_addr < end; physical_addr += PAGE_SIZE) {
    unsigned int frame = ADDRESS_TO_FRAME (physical_addr);
    low_page_table.entry[PAGE_TABLE_ENTRY (physical_addr)] = make_page_table_entry (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }
  
  /* Map the frame manager data structures. */
  begin = frame_manager_physical_begin ();
  end = frame_manager_physical_end ();
  /* These are also under 4M. */
  for (physical_addr = begin; physical_addr < end; physical_addr += PAGE_SIZE) {
    unsigned int frame = ADDRESS_TO_FRAME (physical_addr);
    low_page_table.entry[PAGE_TABLE_ENTRY (physical_addr)] = make_page_table_entry (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager_mark_as_used (frame);
  }

  switch_to_page_directory (kernel_page_directory_paddr);
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
		unsigned int frame,
		page_privilege_t privilege,
		writable_t writable,
		present_t present)
{
  page_directory_t* page_directory = get_page_directory ();
  page_table_t* page_table = get_page_table (logical_addr);

  if (!page_directory->entry[PAGE_DIRECTORY_ENTRY(logical_addr)].present) {
    /* Allocate a page table. */
    page_directory->entry[PAGE_DIRECTORY_ENTRY (logical_addr)] = make_page_directory_entry (frame_manager_allocate (), PRESENT);
    /* Flush the TLB. */
    asm volatile ("invlpg %0\n" :: "m" (page_table));
    initialize_page_table (page_table);
    /* TODO:  Insert page table into all page directories or detect and fix problems on page fault. */
  }

  page_table->entry[PAGE_TABLE_ENTRY (logical_addr)] = make_page_table_entry (frame, privilege, writable, present);
  /* Flush the TLB. */
  asm volatile ("invlpg %0\n" :: "m" (logical_addr));
}
