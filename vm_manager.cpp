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
#include "system_automaton.hpp"
#include "automaton.hpp"

extern page_directory kernel_page_directory;
extern page_table kernel_page_table;

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
page_directory::initialize (bool all)
{
  kassert (is_aligned (this, PAGE_SIZE));

  // Copy the kernel page directory.
  page_directory* kernel_directory = vm_manager::get_kernel_page_directory ();
  for (size_t k = 0; k < PAGE_ENTRY_COUNT - 1; ++k) {
    if (all || get_address (k, 0) >= KERNEL_VIRTUAL_BASE) {
      entry[k] = kernel_directory->entry[k];
      if (entry[k].present_) {
  	frame_manager::incref (entry[k].frame_);
      }
    }
    else {
      entry[k] = page_directory_entry (0, paging_constants::NOT_PRESENT);
    }
  }

  // Find the frame corresponding to this page directory.
  page_table* page_table = vm_manager::get_page_table (this);
  const size_t table_entry = get_page_table_entry (this);
  frame_t frame = page_table->entry[table_entry].frame_;

  // Map the page directory to itself.
  entry[PAGE_ENTRY_COUNT - 1] = page_directory_entry (frame, paging_constants::PRESENT);
  frame_manager::incref (frame);
}

void
vm_manager::switch_to_directory (frame_t frame)
{
  /* Switch to the page directory. */
  asm ("mov %0, %%cr3\n" :: "g"(frame_to_physical_address (frame)));
}

frame_t
vm_manager::page_directory_frame (void)
{
  return get_page_directory ()->entry[PAGE_ENTRY_COUNT - 1].frame_;
}

void
vm_manager::initialize ()
{
  kassert (system_automaton::system_automaton != 0);

  page_directory* kernel_page_directory = get_kernel_page_directory ();

  // Mark frames that are currently being used and update the logical address space.
  page_directory* pd = get_page_directory ();
  for (size_t i = 0; i < PAGE_ENTRY_COUNT; ++i) {
    if (pd->entry[i].present_) {
      page_table* pt = get_page_table (reinterpret_cast<const void*> (get_address (i, 0)));
      for (size_t j = 0; j < PAGE_ENTRY_COUNT; ++j) {
	if (pt->entry[j].present_) {
	  const void* address = get_address (i, j);
	  // The page directory uses the same page table for low and high memory.  Only process high memory.
	  if (address >= KERNEL_VIRTUAL_BASE) {
	    if (system_automaton::system_automaton->address_in_use (address) || address == kernel_page_directory) {
	      // If the address is in the logical address space, mark the frame as being used.
	      frame_manager::mark_as_used (pt->entry[j].frame_);
	    }
	    else {
	      // Otherwise, unmap it.
	      pt->entry[j] = page_table_entry (0, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
	    }
	  }
	}
      }
    }
  }

  // I don't trust myself.
  // Consequently, I'm going to flush paging to ensure the subsequent memory accesses are correct.
  switch_to_directory (page_directory_frame ());
}

page_directory*
vm_manager::get_kernel_page_directory (void)
{
  return reinterpret_cast<page_directory*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (&kernel_page_directory));
};

void*
vm_manager::get_stub (void)
{
  // Reuse the address space of the page table.
  return reinterpret_cast<void*> (reinterpret_cast<size_t> (KERNEL_VIRTUAL_BASE) + reinterpret_cast<size_t> (&kernel_page_table));
}

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

  // Find or create a page table.
  if (!page_directory->entry[directory_entry].present_) {
    if (logical_addr < KERNEL_VIRTUAL_BASE ||
  	page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_ == kernel_page_directory->entry[PAGE_ENTRY_COUNT - 1].frame_) {
      // The address is in user space or we are using the kernel page directory.
      // Either way, we can just allocate a page table.
      page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), paging_constants::PRESENT);
      // Flush the TLB.
      asm ("invlpg %0\n" :: "m" (*page_table));
      // Initialize the page table.
      page_table->clear ();
    }
    else {
      // We are using a non-kernel page directory and need a kernel page table.
      if (!kernel_page_directory->entry[directory_entry].present_) {
  	// The page table is not present in the kernel.
	// Allocate a page table and map it in both directories.
  	kernel_page_directory->entry[directory_entry] = page_directory_entry (frame_manager::alloc (), paging_constants::PRESENT);
  	page_directory->entry[directory_entry] = kernel_page_directory->entry[directory_entry];
  	frame_manager::incref (page_directory->entry[directory_entry].frame_);
  	// Flush the TLB.
  	asm ("invlpg %0\n" :: "m" (*page_table));
  	// Initialize the page table.
  	page_table->clear ();
      }
      else {
  	// The page table is present in the kernel.
  	// Copy the entry.
  	page_directory->entry[directory_entry] = kernel_page_directory->entry[directory_entry];
  	frame_manager::incref (page_directory->entry[directory_entry].frame_);
  	// Flush the TLB.
  	asm ("invlpg %0\n" :: "m" (*page_table));
      }
    }
  }

  kassert (page_table->entry[table_entry].present_ == paging_constants::NOT_PRESENT);
  page_table->entry[table_entry] = page_table_entry (fr, privilege, writable, paging_constants::PRESENT);
  frame_manager::incref (fr);

  // Flush the TLB.
  asm ("invlpg %0\n" :: "m" (*static_cast<const char*> (logical_addr)));
}

void
vm_manager::unmap (const void* logical_addr)
{
  page_directory* page_directory = get_page_directory ();
  page_table* page_table = get_page_table (logical_addr);
  const size_t directory_entry = get_page_directory_entry (logical_addr);
  const size_t table_entry = get_page_table_entry (logical_addr);

  kassert (page_directory->entry[directory_entry].present_);
  kassert (page_table->entry[table_entry].present_);

  frame_manager::decref (page_table->entry[table_entry].frame_);
  page_table->entry[table_entry] = page_table_entry (0, paging_constants::SUPERVISOR, paging_constants::NOT_WRITABLE, paging_constants::NOT_PRESENT);
  /* Flush the TLB. */
  asm volatile ("invlpg %0\n" :: "m" (*static_cast<const char*> (logical_addr)));
}
