#ifndef __halt_hpp__
#define __halt_hpp__

/*
  File
  ----
  halt.hpp
  
  Description
  -----------
  Halt the machine.

  Authors:
  Justin R. Wilson
*/

#include "interrupts.hpp"

inline void
halt ()
{
  for (;;) {
    interrupts::disable ();
    asm volatile ("hlt");
  }
}


#endif /* __halt_hpp__ */


