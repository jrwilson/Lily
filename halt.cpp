/*
  File
  ----
  halt.c
  
  Description
  -----------
  Functions for halting the machine.

  Authors
  -------
  Justin R. Wilson
*/

#include "halt.hpp"
#include "idt.hpp"

void
halt ()
{
  disable_interrupts ();
  for (;;) {
    asm volatile ("hlt");
  }
}
