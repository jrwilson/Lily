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
#include "frame_manager.hpp"
#include "kassert.hpp"

/* Marks the beginning and end of the kernel upon loading. */
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

page_table_entry::page_table_entry () :
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

page_table_entry::page_table_entry (frame frame,
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

void
page_table::clear (void)
{
  kassert (logical_address (this).is_aligned (PAGE_SIZE));
  
  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    entry[k] = page_table_entry (frame (), paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
  }
}

page_directory_entry::page_directory_entry () :
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

page_directory_entry::page_directory_entry (frame frame,
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

void
page_directory::clear (physical_address address)
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

void
page_directory::initialize_with_current (physical_address address)
{
  kassert (logical_address (this).is_aligned (PAGE_SIZE));
  kassert (address.is_aligned (PAGE_SIZE));

  page_directory* current = vm_manager::get_page_directory ();

  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
    /* Copy. */
    entry[k] = current->entry[k];
    /* Increment the reference count. */
    if (entry[k].present_) {
      frame_manager::incref (frame (entry[k]));
    }
  }

  /* Map the page directory to itself. */
  entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (frame (address), paging_constants::PRESENT);
}

page_directory vm_manager::kernel_page_directory __attribute__ ((aligned (PAGE_SIZE)));
page_table vm_manager::low_page_table __attribute__ ((aligned (PAGE_SIZE)));

void
vm_manager::switch_to_directory (physical_address address)
{
  kassert (address.is_aligned (PAGE_SIZE));

  /* Switch to the page directory. */
  asm volatile ("mov %0, %%eax\n"
  		"mov %%eax, %%cr3\n" :: "r"(address.value ()) : "%eax");
}

page_directory*
vm_manager::get_page_directory (void)
{
  /* Because the page directory is mapped to itself. */
  return reinterpret_cast<page_directory*> (0xFFFFF000);
}

page_table*
vm_manager::get_page_table (logical_address address)
{
  return reinterpret_cast<page_table*> (0xFFC00000 + address.page_directory_entry () * PAGE_SIZE);
}

void
vm_manager::initialize (logical_address placement_begin,
			logical_address placement_end)
{
  physical_address low_page_table_paddr (logical_address (&low_page_table) - KERNEL_VIRTUAL_BASE);
  physical_address kernel_page_directory_paddr (logical_address (&kernel_page_directory) - KERNEL_VIRTUAL_BASE);

  /* Clear the page directory and page table. */
  low_page_table.clear ();
  kernel_page_directory.clear (kernel_page_directory_paddr);

  /* Insert the page table. */
  kernel_page_directory.entry[logical_address (0).page_directory_entry ()] = page_directory_entry (frame (low_page_table_paddr), paging_constants::PRESENT);
  kernel_page_directory.entry[KERNEL_VIRTUAL_BASE.page_directory_entry ()] = page_directory_entry (frame (low_page_table_paddr), paging_constants::PRESENT);

  logical_address begin, end;

  /* Identity map the first 1MB. */
  begin = logical_address (0);
  end = logical_address (reinterpret_cast<void*> (0x100000));
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (reinterpret_cast<size_t> (begin.value ()));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the kernel text. */
  begin = logical_address (&text_begin) >> PAGE_SIZE;
  end = logical_address (&text_end) << PAGE_SIZE;
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the kernel read-only data. */
  begin = logical_address (&rodata_begin) >> PAGE_SIZE;
  end = logical_address (&rodata_end) << PAGE_SIZE;
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the kernel data. */
  begin = logical_address (&data_begin) >> PAGE_SIZE;
  end = logical_address (&data_end) << PAGE_SIZE;
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    frame_manager::mark_as_used (frame);
  }

  /* Map the memory allocated by placement. */
  begin = logical_address (placement_begin) >> PAGE_SIZE;
  end = logical_address (placement_end) << PAGE_SIZE;
  kassert (end < INITIAL_LOGICAL_LIMIT);
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - KERNEL_VIRTUAL_BASE);
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    frame_manager::mark_as_used (frame);
  }

  switch_to_directory (kernel_page_directory_paddr);
}

physical_address
vm_manager::page_directory_physical_address (void)
{
  page_directory* page_directory = get_page_directory ();
  return frame (page_directory->entry[PAGE_ENTRY_COUNT - 1]);
}

logical_address
vm_manager::page_directory_logical_address (void)
{
  return logical_address (get_page_directory ());
}

void
vm_manager::map (logical_address logical_addr,
		 frame fr,
		 paging_constants::page_privilege_t privilege,
		 paging_constants::writable_t writable)
{
  page_directory* page_directory = get_page_directory ();
  page_table* page_table = get_page_table (logical_addr);
  
  if (!page_directory->entry[logical_addr.page_directory_entry ()].present_) {
    if (logical_addr < KERNEL_VIRTUAL_BASE ||
	page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_ == kernel_page_directory.entry[PAGE_ENTRY_COUNT - 1].frame_) {
      /* Either the address is in user space or we are using the kernel page directory. */
      /* Allocate a page table. */
      page_directory->entry[logical_addr.page_directory_entry ()] = page_directory_entry (frame_manager::alloc (), paging_constants::PRESENT);
      /* Flush the TLB. */
      asm volatile ("invlpg %0\n" :: "m" (page_table));
      /* Initialize the page table. */
      page_table->clear ();
    }
    else {
      /* Using a non-kernel page directory but we need a kernel page table. */
      if (!kernel_page_directory.entry[logical_addr.page_directory_entry ()].present_) {
	/* The page table is not present in the kernel. */
	/* Allocate a page table and map it in both directories. */
	kernel_page_directory.entry[logical_addr.page_directory_entry ()] = page_directory_entry (frame_manager::alloc (), paging_constants::PRESENT);
	page_directory->entry[logical_addr.page_directory_entry ()] = kernel_page_directory.entry[logical_addr.page_directory_entry ()];
	frame_manager::incref (frame (page_directory->entry[logical_addr.page_directory_entry ()]));
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
	/* Initialize the page table. */
	page_table->clear ();
      }
      else {
	/* The page table is present in the kernel. */
	/* Copy the entry. */
	page_directory->entry[logical_addr.page_directory_entry ()] = kernel_page_directory.entry[logical_addr.page_directory_entry ()];
	frame_manager::incref (frame (page_directory->entry[logical_addr.page_directory_entry ()]));
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
      }
    }
  }

  page_table->entry[logical_addr.page_table_entry ()] = page_table_entry (fr, privilege, writable, paging_constants::PRESENT);

  /* Flush the TLB. */
  asm volatile ("invlpg %0\n" :: "m" (logical_addr));
}

void
vm_manager::unmap (logical_address logical_addr)
{
  page_directory* page_directory = get_page_directory ();
  page_table* page_table = get_page_table (logical_addr);

  if (page_directory->entry[logical_addr.page_directory_entry ()].present_) {
    page_table->entry[logical_addr.page_table_entry ()] = page_table_entry (frame (), paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
    /* Flush the TLB. */
    asm volatile ("invlpg %0\n" :: "m" (logical_addr));
  }
}
