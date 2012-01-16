#ifndef __registers_hpp__
#define __registers_hpp__

/*
  Exceptions/interrupts/traps push this data structure onto the stack.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

struct registers {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t number;
  uint32_t error;
  uint32_t eip, cs, eflags, useresp, ss;
};

#endif /* __registers_hpp__ */
