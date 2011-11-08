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

#include "idt.h"
#include "vm_manager.h"
#include "automaton.h"
#include "scheduler.h"
#include "system_automaton.h"
#include "kassert.h"
#include "string.h"

/* Macros for page faults. */
#define PAGE_FAULT_INTERRUPT 14
#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

static void
page_fault_handler (registers_t* regs)
{
  /* Get the faulting address. */
  void* addr;
  asm volatile ("mov %%cr2, %0\n" : "=r"(addr));
  addr = (void*)PAGE_ALIGN_DOWN ((size_t)addr);

  automaton_t* automaton;

  if (addr < (void*)KERNEL_VIRTUAL_BASE) {
    /* Get the current automaton. */  
    automaton = scheduler_get_current_automaton ();
  }
  else {
    automaton = system_automaton_get_instance ();
  }

  /* Find the address in the memory map. */
  vm_area_t* ptr;
  for (ptr = automaton->memory_map_begin; ptr != 0; ptr = ptr->next) {
    if (addr >= ptr->begin && addr < ptr->end) {
      break;
    }
  }

  if (ptr != 0) {
    if (regs->error & PAGE_PROTECTION_ERROR) {
      /* Protection error. */
      kassert (0);
    }
    else {
      /* Not present. */
      if (regs->error & PAGE_INSTRUCTION_ERROR) {
  	/* Instruction. */
  	kassert (0);
      }
      else {
  	/* Data. */
  	/* Back the request with a frame. */
  	vm_manager_map (addr, frame_manager_alloc (), ptr->page_privilege, ptr->writable, PRESENT);
  	/* Clear. */
  	memset (addr, 0xFF, PAGE_SIZE);
  	return;
      }
    }
  }
  else {
    /* TODO:  Accessed memory not in their map. Segmentation fault. */
    kassert (0);
  }

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

