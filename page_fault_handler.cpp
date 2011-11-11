/*
  File
  ----
  page_fault_handler.h
  
  Description
  -----------
  Page fault handler.

  Authors:
  Justin R. Wilson
*/

#include "idt.hpp"
#include "vm_manager.hpp"
#include "automaton.hpp"
#include "scheduler.hpp"
#include "system_automaton.hpp"
#include "kassert.hpp"
#include "string.hpp"

/* Macros for page faults. */
#define PAGE_FAULT_INTERRUPT 14

static void
page_fault_handler (registers_t* regs)
{
  /* Get the faulting address. */
  void* addr;
  asm volatile ("mov %%cr2, %0\n" : "=r"(addr));
  addr = (void*)PAGE_ALIGN_DOWN ((size_t)addr);

  automaton_interface* automaton;

  if (addr < (void*)KERNEL_VIRTUAL_BASE) {
    /* Get the current automaton. */  
    automaton = scheduler_get_current_automaton ();
  }
  else {
    automaton = system_automaton_get_instance ();
  }

  automaton->page_fault (addr, regs->error);
  
  /* kputs ("Page fault!!\n"); */
  
  /* kputs ("Address: "); kputp (addr); kputs ("\n"); */
  /* kputs ("Codes: "); */
  /* if (regs->error & PAGE_PROTECTION_ERROR) { */
  /*   kputs ("PROTECTION "); */
  /* } */
  /* else { */
  /*   kputs ("NOT_PRESENT "); */
  /* } */

  /* if (regs->error & PAGE_WRITE_ERROR) { */
  /*   kputs ("WRITE "); */
  /* } */
  /* else { */
  /*   kputs ("READ "); */
  /* } */

  /* if (regs->error & PAGE_USER_ERROR) { */
  /*   kputs ("USER "); */
  /* } */
  /* else { */
  /*   kputs ("SUPERVISOR "); */
  /* } */

  /* if (regs->error & PAGE_RESERVED_ERROR) { */
  /*   kputs ("RESERVED "); */
  /* } */

  /* if (regs->error & PAGE_INSTRUCTION_ERROR) { */
  /*   kputs ("INSTRUCTION "); */
  /* } */
  /* else { */
  /*   kputs ("DATA "); */
  /* } */
  /* kputs ("\n"); */
  
  /* kputs ("CS: "); kputuix (regs->cs); kputs (" EIP: "); kputuix (regs->eip); kputs (" EFLAGS: "); kputuix (regs->eflags); kputs ("\n"); */
  /* kputs ("SS: "); kputuix (regs->ss); kputs (" ESP: "); kputuix (regs->useresp); kputs (" DS:"); kputuix (regs->ds); kputs ("\n"); */
  
  /* kputs ("EAX: "); kputuix (regs->eax); kputs (" EBX: "); kputuix (regs->ebx); kputs (" ECX: "); kputuix (regs->ecx); kputs (" EDX: "); kputuix (regs->edx); kputs ("\n"); */
  /* kputs ("ESP: "); kputuix (regs->esp); kputs (" EBP: "); kputuix (regs->ebp); kputs (" ESI: "); kputuix (regs->esi); kputs (" EDI: "); kputuix (regs->edi); kputs ("\n"); */
  
  /* kputs ("Halting"); */
  /* halt (); */
}

void
page_fault_handler_initialize (void)
{
  set_interrupt_handler (PAGE_FAULT_INTERRUPT, page_fault_handler);
}

