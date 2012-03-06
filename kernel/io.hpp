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

  inline uint16_t
  inw (uint16_t port)
  {
    uint16_t value;
    asm ("inw %1, %0" : "=a" (value) : "dN" (port));
    return value;
  }
  
  inline void
  outw (uint16_t port,
	uint16_t value)
  {
    asm ("outw %1, %0" : : "dN" (port), "a" (value));
  }

  inline uint32_t
  inl (uint16_t port)
  {
    uint32_t value;
    asm ("inl %1, %0" : "=a" (value) : "dN" (port));
    return value;
  }
  
  inline void
  outl (uint16_t port,
	uint32_t value)
  {
    asm ("outl %1, %0" : : "dN" (port), "a" (value));
  }

}

#endif /* __io_hpp__ */
