/*
  File
  ----
  kmain.cpp
  
  Description
  -----------
  Main function of the kernel.

  Authors
  -------
  http://wiki.osdev.org/Bare_bones
  Justin R. Wilson
*/

#include "kout.hpp"
#include "multiboot_parser.hpp"
#include "system_allocator.hpp"
#include "vm.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "frame_manager.hpp"
#include "system_automaton.hpp"
#include "scheduler.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

extern int* ctors_begin;
extern int* ctors_end;

// Mark frames and logical addresses when rectifying virtual memory.
static void
mark (logical_address_t begin,
      logical_address_t end,
      bool should_appear,
      vm::writable_t writable)
{
  for (; begin < end; begin += PAGE_SIZE) {
    // Prevent the frame manager from allocating this frame.
    frame_manager::mark_as_used (physical_address_to_frame (begin - KERNEL_VIRTUAL_BASE));
    if (should_appear && begin < INITIAL_LOGICAL_LIMIT) {
      // Correct permissions.
      vm::remap (begin, vm::USER, writable);
      // Mark.
      vm::set_available (begin, 1);
    }
  }
}

static void
sweep ()
{
  for (logical_address_t address = KERNEL_VIRTUAL_BASE; address != INITIAL_LOGICAL_LIMIT; address += PAGE_SIZE) {
    if (vm::get_available (address) == 0) {
      // Unmap addresses that aren't marked.
      vm::unmap (address, false);
    }
    else {
      // Reset.
      vm::set_available (address, 0);
    }
  }
}

extern "C" void
kmain (uint32_t multiboot_magic,
       multiboot_info_t* multiboot_info)  // Physical address.
{
  kout.initialize ();

  // Print a welcome message.
  kout << "Lily" << endl;

  physical_address_t initrd_begin;
  physical_address_t initrd_end;
  
  {
    // A wrapper around multiboot_info_t.
    // It is scoped to prevent using it after frames are reclaimed when correcting the virtual memory map.
    const multiboot_parser multiboot_parser (multiboot_magic, multiboot_info);
    if (!multiboot_parser.okay ()) {
      kout << "No multiboot information.  Halting." << endl;
      halt ();
    }
    if (!multiboot_parser.has_memory_map ()) {
      kout << "Multiboot did not provide a memory map.  Halting." << endl;
      halt ();
    }
    if (!multiboot_parser.has_module_info ()) {
      kout << "Multiboot did not provide module info.  Halting." << endl;
      halt ();
    }
    if (multiboot_parser.module_count () != 1) {
      kout << "Exactly one module is required for an initial ramdisk.  Halting." << endl;
      halt ();
    }

    // Record the physical beginning and end of the module.
    const multiboot_module_t* mod = multiboot_parser.module_begin ();
    initrd_begin = mod->mod_start;
    initrd_end = mod->mod_end;

    // Calculate the beginning of the heap.
    // The multiboot_parser operates on physical addresses, hence, the conversion from logical to physical.
    // The start of the heap must be at least &data_end.
    // Any multiboot data placed beyond INITIAL_LOGICAL_LIMIT - KERNEL_VIRTUAL_BASE can be ignored.
    // Thus, the start of the heap is one past the largest address used by &data_end or a multiboot data structure that is less than INITIAL_LOGICAL_LIMIT - KERNEL_VIRTUAL_BASE.
    // Page aligned of course.
    const logical_address_t heap_begin = align_up (multiboot_parser.physical_end (reinterpret_cast<logical_address_t> (&data_end) - KERNEL_VIRTUAL_BASE, INITIAL_LOGICAL_LIMIT - KERNEL_VIRTUAL_BASE) + KERNEL_VIRTUAL_BASE, PAGE_SIZE);

    // We also mark the end to make sure we don't run out of memory.
    const logical_address_t heap_end = INITIAL_LOGICAL_LIMIT;

    // Initialize the heap.
    system_alloc::initialize (heap_begin, heap_end);

    // Call the static constructors.
    // Since heap allocation is working, static objects can use dynamic memory.
    // Very convenient.
    for (int** ptr = &ctors_begin; ptr != &ctors_end; ++ptr) {
      void (*ctor) () = reinterpret_cast<void (*) ()> (*ptr);
      ctor ();
    }

    // Set up segmentation for x86, or rather, ignore it.
    kout << "Installing GDT" << endl;
    gdt::install ();
    
    // Set up interrupt dispatching.
    kout << "Installing IDT" << endl;
    idt::install ();  
    kout << "Installing exception handler" << endl;
    exception_handler::install ();
    kout << "Installing irq handler" << endl;
    irq_handler::install ();
    kout << "Installing trap handler" << endl;
    trap_handler::install ();
    
    // Initialize the system that allocates frames (physical pages of memory).
    kout << "Memory map:" << endl;
    for (multiboot_parser::memory_map_iterator pos = multiboot_parser.memory_map_begin ();
  	 pos != multiboot_parser.memory_map_end ();
  	 ++pos) {
      kout << hexformat (static_cast<unsigned long> (pos->addr)) << "-" << hexformat (static_cast<unsigned long> (pos->addr + pos->len - 1));
      switch (pos->type) {
      case MULTIBOOT_MEMORY_AVAILABLE:
  	kout << " AVAILABLE" << endl;
  	{
	  // Intersect the region of memory with usable memory.
	  uint64_t begin = std::max (static_cast<multiboot_uint64_t> (USABLE_MEMORY_BEGIN), pos->addr);
  	  uint64_t end = std::min (static_cast<multiboot_uint64_t> (USABLE_MEMORY_END), pos->addr + pos->len);
  	  begin = align_down (begin, PAGE_SIZE);
  	  end = align_up (end, PAGE_SIZE);
  	  if (begin < end) {
  	    frame_manager::add (pos->addr, pos->addr + pos->len);
  	  }
  	}
  	break;
      case MULTIBOOT_MEMORY_RESERVED:
  	kout << " RESERVED" << endl;
  	break;
      default:
  	kout << " UNKNOWN" << endl;
  	break;
      }
    }
  }

  // Mark frames and logical addresses that are in use.
  mark (KERNEL_VIRTUAL_BASE,
	KERNEL_VIRTUAL_BASE + ONE_MEGABYTE,
	true,
	vm::WRITABLE);

  mark (reinterpret_cast<logical_address_t> (&kernel_page_directory) + KERNEL_VIRTUAL_BASE,
	reinterpret_cast<logical_address_t> (&kernel_page_directory) + KERNEL_VIRTUAL_BASE + sizeof (vm::page_directory),
	true,
	vm::WRITABLE);
  
  mark (reinterpret_cast<logical_address_t> (&kernel_page_table) + KERNEL_VIRTUAL_BASE,
	reinterpret_cast<logical_address_t> (&kernel_page_table) + KERNEL_VIRTUAL_BASE + sizeof (vm::page_table),
	false,
	vm::WRITABLE);
  
  mark (reinterpret_cast<logical_address_t> (&zero_page) + KERNEL_VIRTUAL_BASE,
	reinterpret_cast<logical_address_t> (&zero_page) + KERNEL_VIRTUAL_BASE + PAGE_SIZE,
	false,
	vm::NOT_WRITABLE);
  
  mark (reinterpret_cast<logical_address_t> (&text_begin),
	reinterpret_cast<logical_address_t> (&text_end),
	true,
	vm::NOT_WRITABLE);
  
  mark (reinterpret_cast<logical_address_t> (&rodata_begin),
	reinterpret_cast<logical_address_t> (&rodata_end),
	true,
	vm::NOT_WRITABLE);
  
  mark (reinterpret_cast<logical_address_t> (&data_begin),
	reinterpret_cast<logical_address_t> (&data_end),
	true,
	vm::WRITABLE);
  
  mark (system_alloc::heap_begin (),
	system_alloc::heap_end (),
	true,
	vm::WRITABLE);
  
  mark (initrd_begin + KERNEL_VIRTUAL_BASE,
	initrd_end + KERNEL_VIRTUAL_BASE,
	false,
	vm::NOT_WRITABLE);
  
  // Sweep away logical addresses that aren't in use.
  sweep ();

  // Tell the system allocator that is must allocate frames and map them.
  system_alloc::engage_vm (PAGING_AREA);

  // Create the system automaton.
  system_automaton::create_system_automaton ();

  // Start the scheduler.  Doesn't return.
  scheduler::finish (false, -1, 0);
}
