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
#include "interrupts.hpp"

void
halt ()
{
  interrupts::disable ();
  for (;;) {
    asm volatile ("hlt");
  }
}
