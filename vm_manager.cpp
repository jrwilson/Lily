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

extern page_directory kernel_page_directory;

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

page_table_entry::page_table_entry (frame_t frame,
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
  frame_ (frame)
{ }

void
page_table::clear (void)
{
  kassert (is_aligned (this, PAGE_SIZE));
  
  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT; ++k) {
    entry[k] = page_table_entry (0, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
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

page_directory_entry::page_directory_entry (frame_t frame,
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
  frame_ (frame)
{ }

void
page_directory::initialize_with_current (physical_address_t address)
{
  kassert (is_aligned (this, PAGE_SIZE));
  kassert (is_aligned (address, PAGE_SIZE));

  page_directory* current = vm_manager::get_page_directory ();

  unsigned int k;
  for (k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
    /* Copy. */
    entry[k] = current->entry[k];
    /* Increment the reference count. */
    if (entry[k].present_) {
      frame_manager::incref (entry[k].frame_);
    }
  }

  /* Map the page directory to itself. */
  entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (physical_address_to_frame (address), paging_constants::PRESENT);
}

void
vm_manager::switch_to_directory (physical_address_t address)
{
  kassert (is_aligned (address, PAGE_SIZE));

  /* Switch to the page directory. */
  asm volatile ("mov %0, %%cr3\n" :: "r"(address));
}

physical_address_t
vm_manager::page_directory_physical_address (void)
{
  return frame_to_physical_address (get_page_directory ()->entry[PAGE_ENTRY_COUNT - 1].frame_);
}

page_directory*
vm_manager::get_kernel_page_directory (void)
{
  return reinterpret_cast<page_directory*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (&kernel_page_directory));
};

page_directory*
vm_manager::get_page_directory (void)
{
  /* Because the page directory is mapped to itself. */
  return reinterpret_cast<page_directory*> (0xFFFFF000);
}

page_table*
vm_manager::get_page_table (const void* address)
{
  return reinterpret_cast<page_table*> (0xFFC00000 + get_page_directory_entry (address) * PAGE_SIZE);
}

void
vm_manager::map (const void* logical_addr,
		 frame_t fr,
		 paging_constants::page_privilege_t privilege,
		 paging_constants::writable_t writable)
{
  page_directory* kernel_page_directory = get_kernel_page_directory ();
  page_directory* page_directory = get_page_directory ();
  page_table* page_table = get_page_table (logical_addr);
  const size_t directory_entry = get_page_directory_entry (logical_addr);
  const size_t table_entry = get_page_table_entry (logical_addr);

  if (!page_directory->entry[get_page_directory_entry (logical_addr)].present_) {
    if (logical_addr < KERNEL_VIRTUAL_BASE ||
  	page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_ == kernel_page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_) {
      /* Either the address is in user space or we are using the kernel page directory. */
      /* Allocate a page table. */
      page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), paging_constants::PRESENT);
      /* Flush the TLB. */
      asm volatile ("invlpg %0\n" :: "m" (page_table));
      /* Initialize the page table. */
      page_table->clear ();
    }
    else {
      /* Using a non-kernel page directory but we need a kernel page table. */
      if (!kernel_page_directory->entry[directory_entry].present_) {
  	/* The page table is not present in the kernel. */
  	/* Allocate a page table and map it in both directories. */
  	kernel_page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), paging_constants::PRESENT);
  	page_directory->entry[directory_entry] = kernel_page_directory->entry[directory_entry];
  	frame_manager::incref (page_directory->entry[directory_entry].frame_);
  	/* Flush the TLB. */
  	asm volatile ("invlpg %0\n" :: "m" (page_table));
  	/* Initialize the page table. */
  	page_table->clear ();
      }
      else {
  	/* The page table is present in the kernel. */
  	/* Copy the entry. */
  	page_directory->entry[directory_entry] = kernel_page_directory->entry[directory_entry];
  	frame_manager::incref (page_directory->entry[directory_entry].frame_);
  	/* Flush the TLB. */
  	asm volatile ("invlpg %0\n" :: "m" (page_table));
      }
    }
  }

  page_table->entry[table_entry] = page_table_entry (fr, privilege, writable, paging_constants::PRESENT);

  /* Flush the TLB. */
  asm volatile ("invlpg %0\n" :: "m" (logical_addr));
}

void
vm_manager::unmap (const void* logical_addr)
{
  page_directory* page_directory = get_page_directory ();
  page_table* page_table = get_page_table (logical_addr);
  const size_t directory_entry = get_page_directory_entry (logical_addr);
  const size_t table_entry = get_page_table_entry (logical_addr);

  if (page_directory->entry[directory_entry].present_) {
    page_table->entry[table_entry] = page_table_entry (0, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
    /* Flush the TLB. */
    asm volatile ("invlpg %0\n" :: "m" (logical_addr));
  }
}
