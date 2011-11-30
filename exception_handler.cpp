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

exception_handler::exception_handler (interrupt_descriptor_table& idt)
{
  idt.set (0, make_interrupt_gate (exception0, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (1, make_interrupt_gate (exception1, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (2, make_interrupt_gate (exception2, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (3, make_interrupt_gate (exception3, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (4, make_interrupt_gate (exception4, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (5, make_interrupt_gate (exception5, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (6, make_interrupt_gate (exception6, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (7, make_interrupt_gate (exception7, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (8, make_interrupt_gate (exception8, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (9, make_interrupt_gate (exception9, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (10, make_interrupt_gate (exception10, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (11, make_interrupt_gate (exception11, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (12, make_interrupt_gate (exception12, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (13, make_interrupt_gate (exception13, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (14, make_interrupt_gate (exception14, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (15, make_interrupt_gate (exception15, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (16, make_interrupt_gate (exception16, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (17, make_interrupt_gate (exception17, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (18, make_interrupt_gate (exception18, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (19, make_interrupt_gate (exception19, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (20, make_interrupt_gate (exception20, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (21, make_interrupt_gate (exception21, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (22, make_interrupt_gate (exception22, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (23, make_interrupt_gate (exception23, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (24, make_interrupt_gate (exception24, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (25, make_interrupt_gate (exception25, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (26, make_interrupt_gate (exception26, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (27, make_interrupt_gate (exception27, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (28, make_interrupt_gate (exception28, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (29, make_interrupt_gate (exception29, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (30, make_interrupt_gate (exception30, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
  idt.set (31, make_interrupt_gate (exception31, KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
}

extern "C" void
exception_handler (registers regs)
{
  kputs ("Unhandled exception!\n");
  kputs ("Interrupt: "); kputx32 (regs.number); kputs (" Code: " ); kputx32 (regs.error); kputs ("\n");
  
  kputs ("CS: "); kputx32 (regs.cs); kputs (" EIP: "); kputx32 (regs.eip); kputs (" EFLAGS: "); kputx32 (regs.eflags); kputs ("\n");
  kputs ("SS: "); kputx32 (regs.ss); kputs (" ESP: "); kputx32 (regs.useresp); kputs (" DS:"); kputx32 (regs.ds); kputs ("\n");
  
  kputs ("EAX: "); kputx32 (regs.eax); kputs (" EBX: "); kputx32 (regs.ebx); kputs (" ECX: "); kputx32 (regs.ecx); kputs (" EDX: "); kputx32 (regs.edx); kputs ("\n");
  kputs ("ESP: "); kputx32 (regs.esp); kputs (" EBP: "); kputx32 (regs.ebp); kputs (" ESI: "); kputx32 (regs.esi); kputs (" EDI: "); kputx32 (regs.edi); kputs ("\n");
  
  halt ();
}

//   /* Macros for page faults. */
// #define PAGE_FAULT_INTERRUPT 14

// template <class AllocatorTag, template <typename> class Allocator>
// class page_fault_handler {
// private:
//   static void
//   handler (interrupt_descriptor_table::registers* regs)
//   {
//     /* Get the faulting address. */
//     void* addr;
//     asm volatile ("mov %%cr2, %0\n" : "=r"(addr));
    
//     logical_address address (addr);
    
//     automaton_interface* automaton;
    
//     if (address < KERNEL_VIRTUAL_BASE) {
//       /* Get the current automaton. */  
//       automaton = scheduler<AllocatorTag, Allocator>::get_current_automaton ();
//     }
//     else {
//       automaton = system_automaton<AllocatorTag, Allocator>::get_instance ();
//     }
    
//     automaton->page_fault (address, regs->error);
    
//     /* kputs ("Page fault!!\n"); */
    
//     /* kputs ("Address: "); kputp (addr); kputs ("\n"); */
//     /* kputs ("Codes: "); */
//     /* if (regs->error & PAGE_PROTECTION_ERROR) { */
//     /*   kputs ("PROTECTION "); */
//     /* } */
//     /* else { */
//     /*   kputs ("NOT_PRESENT "); */
//     /* } */
    
//     /* if (regs->error & PAGE_WRITE_ERROR) { */
//     /*   kputs ("WRITE "); */
//     /* } */
//     /* else { */
//     /*   kputs ("READ "); */
//     /* } */
    
//     /* if (regs->error & PAGE_USER_ERROR) { */
//     /*   kputs ("USER "); */
//     /* } */
//     /* else { */
//     /*   kputs ("SUPERVISOR "); */
//     /* } */
    
//     /* if (regs->error & PAGE_RESERVED_ERROR) { */
//     /*   kputs ("RESERVED "); */
//     /* } */
    
//     /* if (regs->error & PAGE_INSTRUCTION_ERROR) { */
//     /*   kputs ("INSTRUCTION "); */
//     /* } */
//     /* else { */
//     /*   kputs ("DATA "); */
//     /* } */
//     /* kputs ("\n"); */
    
//     /* kputs ("CS: "); kputuix (regs->cs); kputs (" EIP: "); kputuix (regs->eip); kputs (" EFLAGS: "); kputuix (regs->eflags); kputs ("\n"); */
//     /* kputs ("SS: "); kputuix (regs->ss); kputs (" ESP: "); kputuix (regs->useresp); kputs (" DS:"); kputuix (regs->ds); kputs ("\n"); */
    
//     /* kputs ("EAX: "); kputuix (regs->eax); kputs (" EBX: "); kputuix (regs->ebx); kputs (" ECX: "); kputuix (regs->ecx); kputs (" EDX: "); kputuix (regs->edx); kputs ("\n"); */
//     /* kputs ("ESP: "); kputuix (regs->esp); kputs (" EBP: "); kputuix (regs->ebp); kputs (" ESI: "); kputuix (regs->esi); kputs (" EDI: "); kputuix (regs->edi); kputs ("\n"); */
    
//     /* kputs ("Halting"); */
//     /* halt (); */
//   }

// public:  
//   static void
//   initialize (void)
//   {
//     set_interrupt_handler (PAGE_FAULT_INTERRUPT, handler);
//   }
// };
