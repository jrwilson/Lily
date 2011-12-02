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
#include "vm_def.hpp"

/* Marks the beginning and end of the kernel upon loading. */
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

void
vm_manager::switch_to_directory (physical_address address)
{
  kassert (address.is_aligned (PAGE_SIZE));

  /* Switch to the page directory. */
  asm volatile ("mov %0, %%eax\n"
  		"mov %%eax, %%cr3\n" :: "r"(address.value ()) : "%eax");
}

vm_manager::vm_manager (logical_address placement_begin,
			logical_address placement_end,
			frame_manager& fm) :
  frame_manager_ (fm)
{
  physical_address low_page_table_paddr (logical_address (&low_page_table) - logical_address (KERNEL_VIRTUAL_BASE));
  physical_address kernel_page_directory_paddr (logical_address (&kernel_page_directory) - logical_address (KERNEL_VIRTUAL_BASE));

  /* Clear the page directory and page table. */
  low_page_table.clear ();
  kernel_page_directory.clear (kernel_page_directory_paddr);

  /* Insert the page table. */
  kernel_page_directory.entry[logical_address (0).page_directory_entry ()] = page_directory_entry (frame (low_page_table_paddr), paging_constants::PRESENT);
  kernel_page_directory.entry[logical_address (KERNEL_VIRTUAL_BASE).page_directory_entry ()] = page_directory_entry (frame (low_page_table_paddr), paging_constants::PRESENT);

  logical_address begin, end;

  /* Identity map the first 1MB. */
  begin = logical_address (0);
  end = logical_address (reinterpret_cast<void*> (0x100000));
  kassert (end < logical_address (INITIAL_LOGICAL_LIMIT));
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (reinterpret_cast<size_t> (begin.value ()));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    fm.mark_as_used (frame);
  }

  /* Map the kernel text. */
  begin = logical_address (&text_begin) >> PAGE_SIZE;
  end = logical_address (&text_end) << PAGE_SIZE;
  kassert (end < logical_address (INITIAL_LOGICAL_LIMIT));
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - logical_address (KERNEL_VIRTUAL_BASE));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::PRESENT);
    fm.mark_as_used (frame);
  }

  /* Map the kernel read-only data. */
  begin = logical_address (&rodata_begin) >> PAGE_SIZE;
  end = logical_address (&rodata_end) << PAGE_SIZE;
  kassert (end < logical_address (INITIAL_LOGICAL_LIMIT));
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - logical_address (KERNEL_VIRTUAL_BASE));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    fm.mark_as_used (frame);
  }

  /* Map the kernel data. */
  begin = logical_address (&data_begin) >> PAGE_SIZE;
  end = logical_address (&data_end) << PAGE_SIZE;
  kassert (end < logical_address (INITIAL_LOGICAL_LIMIT));
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - logical_address (KERNEL_VIRTUAL_BASE));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    fm.mark_as_used (frame);
  }

  /* Map the memory allocated by placement. */
  begin = logical_address (placement_begin) >> PAGE_SIZE;
  end = logical_address (placement_end) << PAGE_SIZE;
  kassert (end < logical_address (INITIAL_LOGICAL_LIMIT));
  for (; begin < end; begin += PAGE_SIZE) {
    physical_address pa (begin - logical_address (KERNEL_VIRTUAL_BASE));
    frame frame (pa);
    low_page_table.entry[begin.page_table_entry ()] = page_table_entry (frame, paging_constants::SUPERVISOR, paging_constants::WRITABLE, paging_constants::PRESENT);
    fm.mark_as_used (frame);
  }

  switch_to_directory (kernel_page_directory_paddr);
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
    if (logical_addr < logical_address (KERNEL_VIRTUAL_BASE) ||
	page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_ == kernel_page_directory.entry[PAGE_ENTRY_COUNT - 1].frame_) {
      /* Either the address is in user space or we are using the kernel page directory. */
      /* Allocate a page table. */
      page_directory->entry[logical_addr.page_directory_entry ()] = page_directory_entry (frame_manager_.alloc (), paging_constants::PRESENT);
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
	kernel_page_directory.entry[logical_addr.page_directory_entry ()] = page_directory_entry (frame_manager_.alloc (), paging_constants::PRESENT);
	page_directory->entry[logical_addr.page_directory_entry ()] = kernel_page_directory.entry[logical_addr.page_directory_entry ()];
	frame_manager_.incref (frame (page_directory->entry[logical_addr.page_directory_entry ()]));
	/* Flush the TLB. */
	asm volatile ("invlpg %0\n" :: "m" (page_table));
	/* Initialize the page table. */
	page_table->clear ();
      }
      else {
	/* The page table is present in the kernel. */
	/* Copy the entry. */
	page_directory->entry[logical_addr.page_directory_entry ()] = kernel_page_directory.entry[logical_addr.page_directory_entry ()];
	frame_manager_.incref (frame (page_directory->entry[logical_addr.page_directory_entry ()]));
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

void
page_directory::initialize_with_current (frame_manager& fm,
					 physical_address address)
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
      fm.incref (frame (entry[k]));
    }
  }

  /* Map the page directory to itself. */
  entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (frame (address), paging_constants::PRESENT);
}
