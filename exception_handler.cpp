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
#include "scheduler.hpp"
#include "system_automaton.hpp"

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
  idt::set (0, make_interrupt_gate (exception0, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (1, make_interrupt_gate (exception1, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (2, make_interrupt_gate (exception2, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (3, make_interrupt_gate (exception3, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (4, make_interrupt_gate (exception4, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (5, make_interrupt_gate (exception5, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (6, make_interrupt_gate (exception6, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (7, make_interrupt_gate (exception7, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (8, make_interrupt_gate (exception8, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (9, make_interrupt_gate (exception9, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (10, make_interrupt_gate (exception10, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (11, make_interrupt_gate (exception11, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (12, make_interrupt_gate (exception12, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (13, make_interrupt_gate (exception13, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (14, make_interrupt_gate (exception14, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (15, make_interrupt_gate (exception15, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (16, make_interrupt_gate (exception16, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (17, make_interrupt_gate (exception17, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (18, make_interrupt_gate (exception18, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (19, make_interrupt_gate (exception19, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (20, make_interrupt_gate (exception20, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (21, make_interrupt_gate (exception21, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (22, make_interrupt_gate (exception22, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (23, make_interrupt_gate (exception23, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (24, make_interrupt_gate (exception24, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (25, make_interrupt_gate (exception25, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (26, make_interrupt_gate (exception26, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (27, make_interrupt_gate (exception27, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (28, make_interrupt_gate (exception28, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (29, make_interrupt_gate (exception29, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (30, make_interrupt_gate (exception30, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt::set (31, make_interrupt_gate (exception31, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
}

static const unsigned int DIVIDE_ERROR = 0;
static const unsigned int SINGLE_STEP = 1;
static const unsigned int NON_MASKABLE_INTERRUPT = 2;
static const unsigned int BREAKPOINT = 3;
static const unsigned int OVERFLOW = 4;
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
    // TODO
    kassert (0);
    break;
  case SINGLE_STEP:
    // TODO
    kassert (0);
    break;
  case NON_MASKABLE_INTERRUPT:
    // TODO
    kassert (0);
    break;
  case BREAKPOINT:
    // TODO
    kassert (0);
    break;
  case OVERFLOW:
    // TODO
    kassert (0);
    break; 
  case BOUND:
    // TODO
    kassert (0);
    break; 
  case INVALID_OPCODE:
    // TODO
    kassert (0);
    break; 
  case COPROCESSOR_NA:
    // TODO
    kassert (0);
    break; 
  case DOUBLE_FAULT:
    // TODO
    kassert (0);
    break; 
  case COPROCESSOR_SEGMENT_OVERRUN:
    // TODO
    kassert (0);
    break; 
  case INVALID_TASK_STATE_SEGMENT:
    // TODO
    kassert (0);
    break; 
  case SEGMENT_NOT_PRESENT:
    // TODO
    kassert (0);
    break; 
  case STACK_SEGMENT_OVERRUN:
    // TODO
    kassert (0);
    break; 
  case GENERAL_PROTECTION_FAULT:
    // TODO
    kout << "General Protection Fault" << endl;
    kassert (0);
    break; 
  case PAGE_FAULT:
    {
      // Get the faulting address.
      const void* address;
      asm ("mov %%cr2, %0\n" : "=g"(address));
      if (address < KERNEL_VIRTUAL_BASE) {
	// Use the automaton's memory map.
	scheduler::current_automaton ()->page_fault (address, regs.error, &regs);
      }
      else {
	// Use our memory map.
	system_automaton::system_automaton->page_fault (address, regs.error, &regs);
      }
    }
    break; 
  case COPROCESSOR_ERROR:
    // TODO
    kassert (0);
    break; 
  }
}
