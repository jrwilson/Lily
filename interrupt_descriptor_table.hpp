#ifndef __interrupt_descriptor_table_hpp__
#define __interrupt_descriptor_table_hpp__

/*
  File
  ----
  interrupt_descriptor_table.hpp
  
  Description
  -----------
  Interrupt descriptor table (IDT) object.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

#include "descriptor.hpp"
#include <stdint.h>

typedef uint16_t interrupt_number;

struct registers {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t number;
  uint32_t error;
  uint32_t eip, cs, eflags, useresp, ss;
};

namespace interrupt_descriptor_table {
  void
  install ();

  interrupt_descriptor
  get (interrupt_number k);

  void
  set (interrupt_number k,
       interrupt_descriptor id);
};

#endif /* __interrupt_descriptor_table_hpp__ */
