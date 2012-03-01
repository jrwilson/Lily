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

static inline void __attribute__((noreturn))
halt (void)
{
  for (;;) {
    asm volatile ("cli\n"
		  "hlt\n");
  }
}


#endif /* __halt_hpp__ */


