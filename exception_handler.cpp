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
#include "interrupt_descriptor_table.hpp"
#include "gdt.hpp"
#include "system_automaton.hpp"
#include "kassert.hpp"

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
  interrupt_descriptor_table::set (0, make_interrupt_gate (exception0, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (1, make_interrupt_gate (exception1, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (2, make_interrupt_gate (exception2, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (3, make_interrupt_gate (exception3, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (4, make_interrupt_gate (exception4, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (5, make_interrupt_gate (exception5, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (6, make_interrupt_gate (exception6, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (7, make_interrupt_gate (exception7, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (8, make_interrupt_gate (exception8, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (9, make_interrupt_gate (exception9, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (10, make_interrupt_gate (exception10, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (11, make_interrupt_gate (exception11, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (12, make_interrupt_gate (exception12, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (13, make_interrupt_gate (exception13, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (14, make_interrupt_gate (exception14, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (15, make_interrupt_gate (exception15, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (16, make_interrupt_gate (exception16, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (17, make_interrupt_gate (exception17, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (18, make_interrupt_gate (exception18, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (19, make_interrupt_gate (exception19, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (20, make_interrupt_gate (exception20, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (21, make_interrupt_gate (exception21, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (22, make_interrupt_gate (exception22, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (23, make_interrupt_gate (exception23, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (24, make_interrupt_gate (exception24, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (25, make_interrupt_gate (exception25, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (26, make_interrupt_gate (exception26, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (27, make_interrupt_gate (exception27, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (28, make_interrupt_gate (exception28, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (29, make_interrupt_gate (exception29, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (30, make_interrupt_gate (exception30, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  interrupt_descriptor_table::set (31, make_interrupt_gate (exception31, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
}

static const interrupt_number DIVIDE_ERROR = 0;
static const interrupt_number SINGLE_STEP = 1;
static const interrupt_number NON_MASKABLE_INTERRUPT = 2;
static const interrupt_number BREAKPOINT = 3;
static const interrupt_number OVERFLOW = 4;
static const interrupt_number BOUND = 5;
static const interrupt_number INVALID_OPCODE = 6;
static const interrupt_number COPROCESSOR_NA = 7;
static const interrupt_number DOUBLE_FAULT = 8;
static const interrupt_number COPROCESSOR_SEGMENT_OVERRUN = 9;
static const interrupt_number INVALID_TASK_STATE_SEGMENT = 10;
static const interrupt_number SEGMENT_NOT_PRESENT = 11;
static const interrupt_number STACK_SEGMENT_OVERRUN = 12;
static const interrupt_number GENERAL_PROTECTION_FAULT = 13;
static const interrupt_number PAGE_FAULT = 14;
static const interrupt_number COPROCESSOR_ERROR = 16;

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
      const void* addr;
      asm ("mov %%cr2, %0\n" : "=g"(addr));
      system_automaton::page_fault (addr, regs.error, &regs);
    }
    break; 
  case COPROCESSOR_ERROR:
    // TODO
    kassert (0);
    break; 
  }
}
