#ifndef __page_fault_handler_hpp__
#define __page_fault_handler_hpp__

/*
  File
  ----
  page_fault_handler.hpp
  
  Description
  -----------
  Page fault handler.

  Authors:
  Justin R. Wilson
*/

#include "automaton_interface.hpp"
#include "system_automaton.hpp"

  /* Macros for page faults. */
#define PAGE_FAULT_INTERRUPT 14

template <class AllocatorTag, template <typename> class Allocator>
class page_fault_handler {
private:
  static void
  handler (interrupt_descriptor_table::registers* regs)
  {
    /* Get the faulting address. */
    void* addr;
    asm volatile ("mov %%cr2, %0\n" : "=r"(addr));
    
    logical_address address (addr);
    
    automaton_interface* automaton;
    
    if (address < KERNEL_VIRTUAL_BASE) {
      /* Get the current automaton. */  
      automaton = scheduler<AllocatorTag, Allocator>::get_current_automaton ();
    }
    else {
      automaton = system_automaton<AllocatorTag, Allocator>::get_instance ();
    }
    
    automaton->page_fault (address, regs->error);
    
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

public:  
  static void
  initialize (void)
  {
    set_interrupt_handler (PAGE_FAULT_INTERRUPT, handler);
  }
};


#endif /* __page_fault_handler_hpp__ */
