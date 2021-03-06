/*
  File
  ----
  exception_handler.cpp
  
  Description
  -----------
  Object for handling exceptions.

  Authors:
  Justin R. Wilson
*/

#include "exception_handler.hpp"
#include "idt.hpp"
#include "gdt.hpp"
#include "kassert.hpp"
#include "vm_def.hpp"
#include "vm.hpp"
#include "string.hpp"
#include "kernel_alloc.hpp"
#include "scheduler.hpp"

extern "C" void exception0 ();
extern "C" void exception1 ();
extern "C" void exception2 ();
extern "C" void exception3 ();
extern "C" void exception4 ();
extern "C" void exception5 ();
extern "C" void exception6 ();
extern "C" void exception7 ();
extern "C" void exception8 ();
extern "C" void exception9 ();
extern "C" void exception10 ();
extern "C" void exception11 ();
extern "C" void exception12 ();
extern "C" void exception13 ();
extern "C" void exception14 ();
extern "C" void exception15 ();
extern "C" void exception16 ();
extern "C" void exception17 ();
extern "C" void exception18 ();
extern "C" void exception19 ();
extern "C" void exception20 ();
extern "C" void exception21 ();
extern "C" void exception22 ();
extern "C" void exception23 ();
extern "C" void exception24 ();
extern "C" void exception25 ();
extern "C" void exception26 ();
extern "C" void exception27 ();
extern "C" void exception28 ();
extern "C" void exception29 ();
extern "C" void exception30 ();
extern "C" void exception31 ();

void
exception_handler::install ()
{
  idt::set (0, make_interrupt_gate (exception0, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (1, make_interrupt_gate (exception1, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (2, make_interrupt_gate (exception2, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (3, make_interrupt_gate (exception3, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (4, make_interrupt_gate (exception4, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (5, make_interrupt_gate (exception5, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (6, make_interrupt_gate (exception6, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (7, make_interrupt_gate (exception7, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (8, make_interrupt_gate (exception8, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (9, make_interrupt_gate (exception9, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (10, make_interrupt_gate (exception10, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (11, make_interrupt_gate (exception11, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (12, make_interrupt_gate (exception12, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (13, make_interrupt_gate (exception13, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (14, make_interrupt_gate (exception14, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (15, make_interrupt_gate (exception15, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (16, make_interrupt_gate (exception16, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (17, make_interrupt_gate (exception17, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (18, make_interrupt_gate (exception18, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (19, make_interrupt_gate (exception19, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (20, make_interrupt_gate (exception20, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (21, make_interrupt_gate (exception21, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (22, make_interrupt_gate (exception22, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (23, make_interrupt_gate (exception23, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (24, make_interrupt_gate (exception24, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (25, make_interrupt_gate (exception25, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (26, make_interrupt_gate (exception26, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (27, make_interrupt_gate (exception27, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (28, make_interrupt_gate (exception28, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (29, make_interrupt_gate (exception29, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (30, make_interrupt_gate (exception30, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
  idt::set (31, make_interrupt_gate (exception31, gdt::KERNEL_CODE_SELECTOR, descriptor::RING0, descriptor::PRESENT));
}

static const unsigned int DIVIDE_ERROR = 0;
static const unsigned int SINGLE_STEP = 1;
static const unsigned int NON_MASKABLE_INTERRUPT = 2;
static const unsigned int BREAKPOINT = 3;
static const unsigned int OVERFLOW_ERROR = 4;
static const unsigned int BOUND = 5;
static const unsigned int INVALID_OPCODE = 6;
static const unsigned int COPROCESSOR_NA = 7;
static const unsigned int DOUBLE_FAULT = 8;
static const unsigned int COPROCESSOR_SEGMENT_OVERRUN = 9;
static const unsigned int INVALID_TASK_STATE_SEGMENT = 10;
static const unsigned int SEGMENT_NOT_PRESENT = 11;
static const unsigned int STACK_SEGMENT_OVERRUN = 12;
static const unsigned int GENERAL_PROTECTION_FAULT = 13;
static const unsigned int PAGE_FAULT = 14;
static const unsigned int COPROCESSOR_ERROR = 16;

extern "C" void
exception_dispatch (volatile registers regs)
{
  switch (regs.number) {
  case DIVIDE_ERROR:
    kpanic ("TODO:  Handle divide error");
    break;
  case SINGLE_STEP:
    kpanic ("TODO:  Handle single step");
    break;
  case NON_MASKABLE_INTERRUPT:
    kpanic ("TODO:  Handle non-maskable interrupt");
    break;
  case BREAKPOINT:
    kpanic ("TODO:  Handle breakpoint");
    break;
  case OVERFLOW_ERROR:
    kpanic ("TODO:  Handle overflow error");
    break; 
  case BOUND:
    kpanic ("TODO:  Handle bound");
    break; 
  case INVALID_OPCODE:
    kpanic ("TODO:  Handle invalide opcode");
    break; 
  case COPROCESSOR_NA:
    kpanic ("TODO:  Handle coprocessor not available");
    break; 
  case DOUBLE_FAULT:
    kpanic ("TODO:  Handle double fault");
    break; 
  case COPROCESSOR_SEGMENT_OVERRUN:
    kpanic ("TODO:  Handle coprocessor segment overrun");
    break; 
  case INVALID_TASK_STATE_SEGMENT:
    kpanic ("TODO:  Handle invalid task state segment");
    break; 
  case SEGMENT_NOT_PRESENT:
    kpanic ("TODO:  Handle segment not present");
    break; 
  case STACK_SEGMENT_OVERRUN:
    kpanic ("TODO:  Handle stack segment overrun");
    break; 
  case GENERAL_PROTECTION_FAULT:
    kpanic ("TODO:  Handle general protection fault");
    break; 
  case PAGE_FAULT:
    {
      // Get the faulting address.
      logical_address_t address;
      asm ("mov %%cr2, %0\n" : "=r"(address));
      // Get the error.
      vm::page_fault_error_t error = regs.error;

      if (!vm::not_present (error) &&
      	  vm::protection_violation (error) &&
      	  vm::write_context (error) &&
      	  vm::data_context (error) &&
      	  vm::get_copy_on_write (address) == vm::COPY_ON_WRITE) {
      	// Copy-on-write.
	// Is the frame part of a buffer?
	vm::buffer_t buf = vm::get_buffer (address);
	frame_t src_frame = vm::logical_address_to_frame (address);
	if (src_frame != vm::zero_frame () && frame_manager::ref_count (src_frame) == 1) {
	  // The frame is not shared so we can just remap it.
	  vm::remap (address, vm::USER, vm::MAP_READ_WRITE, buf);
	}
	else {
	  // The frame is shared so make a copy.
	  // Allocate a frame.
	  frame_t dst_frame = frame_manager::alloc ();
	  kassert (dst_frame != vm::zero_frame ());
	  // Map it in at the stub.
	  vm::map (vm::get_stub1 (), dst_frame, vm::USER, vm::MAP_READ_WRITE);
	  // Copy.
	  memcpy (reinterpret_cast<void *> (vm::get_stub1 ()), reinterpret_cast<const void*> (align_down (address, PAGE_SIZE)), PAGE_SIZE);
	  // Unmap the source.  Decrement if not a buffer.
	  vm::unmap (address, buf != vm::BUFFER);
	  // Unmap the stub.
	  vm::unmap (vm::get_stub1 ());
	  // Map in the destination with the same buffer status.
	  vm::map (address, dst_frame, vm::USER, vm::MAP_READ_WRITE, true, buf);
	  // Remove the reference from allocation.  The final reference count for dst_frame is 1.
	  frame_manager::decref (dst_frame);

	  // static size_t copy_count = 0;
	  // kout << scheduler::current_action ().action->automaton->aid () << " " <<
	  //   hexformat (address) << " copy_count = " << ++copy_count << " " << dst_frame << " -> " << src_frame << endl;
	}

      	// Done.
      	return;
      }

      if (vm::not_present (error) &&
      	  vm::supervisor_context (error) &&
      	  address >= kernel_alloc::heap_begin () &&
      	  address < kernel_alloc::heap_end ()) {
      	// The kernel expanded since the current page directory was created.
      	// Consequently, we may encounter a page fault when trying to access something in the kernel.
      	// We only need to map the page table, however.
      	const size_t idx = vm::get_page_directory_idx (address);
      	vm::get_page_directory ()->entry[idx] = vm::get_kernel_page_directory ()->entry[idx];
      	asm ("invlpg (%0)\n" : : "r"(address));
      	return;
      }

      kout << "Page Fault" << endl;
      kout << "address = " << hexformat (address) << endl;
      // kout << "not_present = " << vm::not_present (error) << endl;
      kout << regs << endl;

      kpanic ("TODO:  Handle page fault");
    }
    break; 
  case COPROCESSOR_ERROR:
    kpanic ("TODO:  Handle coprocessor error");
    break; 
  }
}
