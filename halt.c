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

#include "halt.h"
#include "idt.h"

void
halt ()
{
  disable_interrupts ();
  for (;;) {
    asm volatile ("hlt");
  }
}
