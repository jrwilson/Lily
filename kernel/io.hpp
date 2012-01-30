#ifndef __io_hpp__
#define __io_hpp__

/*
  File
  ----
  io.h
  
  Description
  -----------
  Functions that manipulate I/O ports.

  Authors:
  http://www.jamesmolloy.co.uk/tutorial_html/3.-The%20Screen.html
  Justin R. Wilson
*/

#include "kout.hpp"

namespace io {

  inline uint8_t
  inb (uint16_t port)
  {
    uint8_t value;
    asm ("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
  }
  
  inline void
  outb (uint16_t port,
	uint8_t value)
  {
    asm ("outb %1, %0" : : "dN" (port), "a" (value));
  }

}

#endif /* __io_hpp__ */
