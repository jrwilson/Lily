/*
  File
  ----
  io.c
  
  Description
  -----------
  Functions for I/O ports.

  Authors
  -------
  http://www.jamesmolloy.co.uk/tutorial_html/3.-The%20Screen.html
  Justin R. Wilson
*/

#include "io.h"

void
outb(unsigned short port,
     unsigned char value)
{
  asm volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}
