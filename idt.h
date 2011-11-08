#ifndef __interrupt_h__
#define __interrupt_h__

/*
  File
  ----
  idt.h
  
  Description
  -----------
  Declarations for functions to manage interrupts.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "types.h"

/* Operating system trap starts at 128. */
#define TRAP_BASE 128

struct registers
{
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t number;
  uint32_t error;
  uint32_t eip, cs, eflags, useresp, ss;
};
typedef struct registers registers_t;
typedef void (*interrupt_handler_t) (registers_t*);

void
idt_initialize (void);

void
enable_interrupts (void);

void
disable_interrupts (void);

void
set_interrupt_handler (uint8_t num,
		       interrupt_handler_t handler);

#endif /* __interrupt_h__ */
