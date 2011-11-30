#ifndef __interrupts_hpp__
#define __interrupts_hpp__

/*
  File
  ----
  interrupts.hpp
  
  Description
  -----------
  Functions to manage interrupts.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/4.-The%20GDT%20and%20IDT.html
  Justin R. Wilson
*/

namespace interrupts {
  
  inline void
  enable ()
  {
    asm volatile ("sti");
  }
  
  inline void
  disable ()
  {
    asm volatile ("cli");
  }
  
}

#endif /* __interrupts_hpp__ */
