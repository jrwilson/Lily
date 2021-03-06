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

#include "multiboot_parser.hpp"
#include "kout.hpp"
#include "kernel_alloc.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "exception_handler.hpp"
#include "irq_handler.hpp"
#include "trap_handler.hpp"
#include "frame_manager.hpp"
#include "vm.hpp"
#include "halt.hpp"
#include "scheduler.hpp"
#include "boot_automaton.hpp"

// Symbols to build the kernel's memory map.
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;

extern char bss_begin;
extern char bss_end;

extern char stack_begin;
extern char stack_end;

extern int* ctors_begin;
extern int* ctors_end;

// Clear the mark on all logical addresses.
static void
clear (void)
{
  for (logical_address_t address = KERNEL_VIRTUAL_BASE; address != INITIAL_LOGICAL_LIMIT; address += PAGE_SIZE) {
    vm::set_accessed (address, false);
  }
}

// Mark frames and logical addresses when rectifying virtual memory.
static void
mark (logical_address_t begin,
      logical_address_t end,
      bool should_appear,
      vm::map_mode_t map_mode)
{
  for (; begin < end; begin += PAGE_SIZE) {
    // Prevent the frame manager from allocating this frame.
    frame_manager::mark_as_used (physical_address_to_frame (begin - KERNEL_VIRTUAL_BASE));
    if (should_appear && begin < INITIAL_LOGICAL_LIMIT) {
      // Correct permissions.
      vm::remap (begin, vm::USER, map_mode);
      // Mark.
      vm::set_accessed (begin, true);
    }
  }
}

static void
sweep ()
{
  for (logical_address_t address = KERNEL_VIRTUAL_BASE; address != INITIAL_LOGICAL_LIMIT; address += PAGE_SIZE) {
    if (vm::get_accessed (address) == false) {
      // Unmap addresses that aren't marked.
      vm::unmap (address, false);
    }
  }
}

extern "C" void
kmain (uint32_t multiboot_magic,
       multiboot_info_t* multiboot_info)  // Physical address.
{
  // Zero uninitialized memory avoiding the stack.
  memset (&bss_begin, 0, &stack_begin - &bss_begin);
  memset (&stack_end, 0, &bss_end - &stack_end);

  kout.initialize ();

  // Print a welcome message.
  kout << "Lily" << endl;

  frame_t boot_automaton_frame;
  size_t boot_automaton_size;
  frame_t boot_data_frame;
  size_t boot_data_size;

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
    if (multiboot_parser.module_count () != 2) {
      kout << "Exactly two modules are required for booting.  Halting." << endl;
      halt ();
    }

    const multiboot_module_t* boot_automaton_module = 0;
    const multiboot_module_t* boot_data_module = 0;

    for (const multiboot_module_t* mod = multiboot_parser.module_begin ();
  	 mod != multiboot_parser.module_end ();
  	 ++mod) {
      if (strcmp (reinterpret_cast<const char*> (mod->cmdline), "boot_automaton") == 0) {
  	boot_automaton_module = mod;
      }
      else if (strcmp (reinterpret_cast<const char*> (mod->cmdline), "boot_data") == 0) {
  	boot_data_module = mod;
      }
    }

    if (boot_automaton_module == 0) {
      kout << "No boot_automaton module.  Halting." << endl;
      halt ();
    }

    if (boot_data_module == 0) {
      kout << "No boot_data module.  Halting." << endl;
      halt ();
    }

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
    kernel_alloc::initialize (heap_begin, heap_end);

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
  	  uint64_t begin = max (static_cast<multiboot_uint64_t> (USABLE_MEMORY_BEGIN), pos->addr);
  	  uint64_t end = min (static_cast<multiboot_uint64_t> (USABLE_MEMORY_END), pos->addr + pos->len);
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

    // Clear the mark on all logical addresses.
    clear ();

    // Mark frames and logical addresses that are in use.
    mark (KERNEL_VIRTUAL_BASE,
  	  KERNEL_VIRTUAL_BASE + ONE_MEGABYTE,
  	  true,
  	  vm::MAP_READ_WRITE);

    mark (reinterpret_cast<logical_address_t> (&kernel_page_directory) + KERNEL_VIRTUAL_BASE,
  	  reinterpret_cast<logical_address_t> (&kernel_page_directory) + KERNEL_VIRTUAL_BASE + sizeof (vm::page_directory),
  	  true,
  	  vm::MAP_READ_WRITE);
  
    mark (reinterpret_cast<logical_address_t> (&kernel_page_table) + KERNEL_VIRTUAL_BASE,
  	  reinterpret_cast<logical_address_t> (&kernel_page_table) + KERNEL_VIRTUAL_BASE + sizeof (vm::page_table),
  	  false,
  	  vm::MAP_READ_WRITE);
  
    mark (reinterpret_cast<logical_address_t> (&zero_page) + KERNEL_VIRTUAL_BASE,
  	  reinterpret_cast<logical_address_t> (&zero_page) + KERNEL_VIRTUAL_BASE + PAGE_SIZE,
  	  false,
  	  vm::MAP_READ_ONLY);
  
    mark (reinterpret_cast<logical_address_t> (&text_begin),
  	  reinterpret_cast<logical_address_t> (&text_end),
  	  true,
  	  vm::MAP_READ_ONLY);
  
    mark (reinterpret_cast<logical_address_t> (&data_begin),
  	  reinterpret_cast<logical_address_t> (&data_end),
  	  true,
  	  vm::MAP_READ_WRITE);
  
    mark (kernel_alloc::heap_begin (),
  	  kernel_alloc::heap_end (),
  	  true,
  	  vm::MAP_READ_WRITE);

    // Mark but don't map the initial automaton.
    mark (boot_automaton_module->mod_start + KERNEL_VIRTUAL_BASE,
  	  boot_automaton_module->mod_end + KERNEL_VIRTUAL_BASE,
  	  false,
  	  vm::MAP_READ_ONLY);

    // Mark but don't map the initial automaton's data.
    mark (boot_data_module->mod_start + KERNEL_VIRTUAL_BASE,
  	  boot_data_module->mod_end + KERNEL_VIRTUAL_BASE,
  	  false,
  	  vm::MAP_READ_ONLY);
    
    // Convert the modules into buffers.
    boot_automaton_frame = physical_address_to_frame (boot_automaton_module->mod_start);
    boot_automaton_size = boot_automaton_module->mod_end - boot_automaton_module->mod_start;
    boot_data_frame = physical_address_to_frame (boot_data_module->mod_start);
    boot_data_size = boot_data_module->mod_end - boot_data_module->mod_start;

    // Sweep away logical addresses that aren't in use.
    sweep ();
  }

  // Tell the system allocator that it must allocate frames and map them.
  kernel_alloc::engage_vm (PAGING_AREA);

  // Initialize the scheduler.
  scheduler::initialize ();

  // This scope causes the text and data_buffer resources to be reclaimed.
  {
    // Create a buffer containing the text of the initial automaton.
    const size_t automaton_frame_end = boot_automaton_frame + align_up (boot_automaton_size, PAGE_SIZE) / PAGE_SIZE;
    shared_ptr<buffer> text = shared_ptr<buffer> (new buffer (0));
    kassert (text.get () != 0);
    for (frame_t frame = boot_automaton_frame; frame != automaton_frame_end; ++frame) {
      text->append_frame (frame);
      // Drop the reference count.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }
    
    // Create a buffer to contain the initial data.
    const size_t data_frame_end = boot_data_frame + align_up (boot_data_size, PAGE_SIZE) / PAGE_SIZE;
    shared_ptr<buffer> data_buffer = shared_ptr<buffer> (new buffer (0));
    kassert (data_buffer.get () != 0);
    for (frame_t frame = boot_data_frame; frame != data_frame_end; ++frame) {
      data_buffer->append_frame (frame);
      // Drop the reference count.
      size_t count = frame_manager::decref (frame);
      kassert (count == 1);
    }
    
    // Create the automaton.
    pair<shared_ptr<automaton>, int> r = automaton::create_automaton (true, text, boot_automaton_size);
    
    if (r.first.get () == 0) {
      kout << "Could not create the boot automaton.  Halting." << endl;
      halt ();
    }
    
    boot_automaton = r.first;
    boot_data = boot_automaton->buffer_create (data_buffer);

    // Schedule the init action.
    const paction* init_action = boot_automaton->find_action (kstring ("init"));
    if (init_action == 0 || (init_action->type != INTERNAL && init_action->type != SYSTEM)) {
      kout << "Boot automaton does not contain an init action.  Halting." << endl;
      halt ();
    }

    scheduler::schedule (caction (boot_automaton, init_action, 0));
  }

  // Start the scheduler.  Doesn't return.
  scheduler::finish (false, -1, -1);
}
