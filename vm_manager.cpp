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

#include "vm_manager.hpp"
#include "kassert.hpp"
#include "frame_manager.hpp"
#include "idt.hpp"

/* Everything must fit into 4MB initialy. */
const logical_address INITIAL_LOGICAL_LIMIT (KERNEL_VIRTUAL_BASE + 0x400000);

/* Marks the beginning and end of the kernel upon loading. */
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

static page_directory_t kernel_page_directory __attribute__ ((aligned (PAGE_SIZE)));
static page_table_t low_page_table __attribute__ ((aligned (PAGE_SIZE)));

static void
page_table_clear (page_table_t* ptr)
{
  kassert (ptr != 0);
  kassert (logical_address (ptr).is_aligned (PAGE_SIZE));

  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    ptr->entry[k] = page_table_entry_t (frame (), SUPERVISOR, NOT_WRITABLE, NOT_PRESENT);
  }
}

static void
page_directory_clear (page_directory_t* ptr,
		      physical_address address)
{
  kassert (ptr != 0);
  kassert (logical_address (ptr).is_aligned (PAGE_SIZE));
  kassert (address.is_aligned (PAGE_SIZE));

  /* Clear the page directory and page table. */
  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
    ptr->entry[k] = page_directory_entry_t (frame (), NOT_PRESENT);
  }

  /* Map the page directory to itself. */
  ptr->entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry_t (frame (address), PRESENT);
}

void
vm_manager_switch_to_directory (physical_address address)
{
  kassert (address.is_aligned (PAGE_SIZE));

  /* Switch to the page directory. */
  asm volatile ("mov %0, %%eax\n"
  		"mov %%eax, %%cr3\n" :: "r"(address.value ()) : "%eax");
}

void
vm_manager_initialize (logical_address placement_begin,
		       logical_address placement_end)
{
  physical_address low_page_table_paddr (logical_address (&low_page_table) - KERNEL_VIRTUAL_BASE);
  physical_address kernel_page_directory_paddr (logical_address (&kernel_page_directory) - KERNEL_VIRTUAL_BASE);

  /* Clear the page directory and page table. */
  page_table_clear (&low_page_table);
  page_directory_clear (&kernel_page_directory, kernel_page_directory_paddr);

  /* Insert the page table. */
  kernel_page_directory.entry[logical_address (0).page_directory_entry ()] = page_directory_entry_t (frame (low_page_table_paddr), PRESENT);
  kernel_page_directory.entry[KERNEL_VIRTUAL_BASE.page_directory_entry ()] = page_directory_entry_t (frame (low_page_table_paddr), PRESENT);

  logical_address begin, end;

  /* Identity map the first 1MB. */
  begin = logical_address (0);
  end = logical_address (reinterpret_cast<void*> (0x100000));
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (reinterpret_cast<size_t> (begin.value ()));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry_t (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the kernel text. */
  begin = logical_address (&text_begin);
  begin.align_down (PAGE_SIZE);
  end = logical_address (&text_end);
  end.align_up (PAGE_SIZE); 
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry_t (frame, SUPERVISOR, NOT_WRITABLE, PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the kernel read-only data. */
  begin = logical_address (&rodata_begin);
  begin.align_down (PAGE_SIZE);
  end = logical_address (&rodata_end);
  end.align_up (PAGE_SIZE);
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry_t (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the kernel data. */
  begin = logical_address (&data_begin);
  begin.align_down (PAGE_SIZE);
  end = logical_address (&data_end);
  end.align_up (PAGE_SIZE);
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry_t (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the memory allocated by placement. */
  begin = logical_address (placement_begin);
  begin.align_down (PAGE_SIZE);
  end = logical_address (placement_end);
  end.align_up (PAGE_SIZE);
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry_t (frame, SUPERVISOR, WRITABLE, PRESENT);
    frame_manager::mark_as_used (frame);
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
get_page_table (logical_address address)
{
  return reinterpret_cast<page_table_t*> (0xFFC00000 + address.page_directory_entry () * PAGE_SIZE);
}

physical_address
vm_manager_page_directory_physical_address (void)
{
  page_directory_t* page_directory = get_page_directory ();
  return frame (page_directory->entry[PAGE_ENTRY_COUNT - 1]);
}

logical_address
vm_manager_page_directory_logical_address (void)
{
  return logical_address (get_page_directory ());
}

void
vm_manager_map (logical_address logical_addr,
		frame fr,
		page_privilege_t privilege,
		writable_t writable)
{
  page_directory_t* page_directory = get_page_directory ();
  page_table_t* page_table = get_page_table (logical_addr);

  if (!page_directory->entry[logical_addr.page_directory_entry ()].present_) {
    if (logical_addr < KERNEL_VIRTUAL_BASE ||
	page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_ == kernel_page_directory.entry[PAGE_ENTRY_COUNT - 1].frame_) {
      /* Either the address is in user space or we are using the kernel page directory. */
      /* Allocate a page table. */
      page_directory->entry[logical_addr.page_directory_entry ()] = page_directory_entry_t (frame_manager::alloc (), PRESENT);
      /* Flush the TLB. */
      asm volatile ("invlpg %0\n" :: "m" (page_table));
      /* Initialize the page table. */
      page_table_clear (page_table);
    }
    else {
      /* Using a non-kernel page directory but we need a kernel page table. */
      if (!kernel_page_directory.entry[logical_addr.page_directory_entry ()].present_) {
	/* The page table is not present in the kernel. */
	/* Allocate a page table and map it in both directories. */
	kernel_page_directory.entry[logical_addr.page_directory_entry ()] = page_directory_entry_t (frame_manager::alloc (), PRESENT);
	page_directory->entry[logical_addr.page_directory_entry ()] = kernel_page_directory.entry[logical_addr.page_directory_entry ()];
	frame_manager::incref (frame (page_directory->entry[logical_addr.page_directory_entry ()]));
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
	/* Initialize the page table. */
	page_table_clear (page_table);
      }
      else {
	/* The page table is present in the kernel. */
	/* Copy the entry. */
	page_directory->entry[logical_addr.page_directory_entry ()] = kernel_page_directory.entry[logical_addr.page_directory_entry ()];
	frame_manager::incref ( frame (page_directory->entry[logical_addr.page_directory_entry ()]));
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
      }
    }
  }

  page_table->entry[logical_addr.page_table_entry ()] = page_table_entry_t (fr, privilege, writable, PRESENT);

  /* Flush the TLB. */
  asm volatile ("invlpg %0\n" :: "m" (logical_addr));
}

void
vm_manager_unmap (logical_address logical_addr)
{
  page_directory_t* page_directory = get_page_directory ();
  page_table_t* page_table = get_page_table (logical_addr);

  if (page_directory->entry[logical_addr.page_directory_entry ()].present_) {
    page_table->entry[logical_addr.page_table_entry ()] = page_table_entry_t (frame (), SUPERVISOR, NOT_WRITABLE, NOT_PRESENT);
    /* Flush the TLB. */
    asm volatile ("invlpg %0\n" :: "m" (logical_addr));
  }
}

void
page_directory_initialize_with_current (page_directory_t* ptr,
					physical_address address)
{
  kassert (ptr != 0);
  kassert (logical_address (ptr).is_aligned (PAGE_SIZE));
  kassert (address.is_aligned (PAGE_SIZE));

  page_directory_t* current = get_page_directory ();

  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
    /* Copy. */
    ptr->entry[k] = current->entry[k];
    /* Increment the reference count. */
    if (ptr->entry[k].present_) {
      frame_manager::incref (frame (ptr->entry[k]));
    }
  }

  /* Map the page directory to itself. */
  ptr->entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry_t (frame (address), PRESENT);
}
