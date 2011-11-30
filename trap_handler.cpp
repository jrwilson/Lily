/*
  File
  ----
  trap_handler.hpp
  
  Description
  -----------
  Object for handling traps.
  
  Authors:
  Justin R. Wilson
*/

#include "trap_handler.hpp"

/* Operating system trap starts at 128. */
static const unsigned int TRAP_BASE = 128;

extern "C" void trap0 ();

trap_handler::trap_handler (interrupt_descriptor_table& idt)
{
  idt.set (TRAP_BASE + 0, make_interrupt_gate (trap0, KERNEL_CODE_SELECTOR, RING0, PRESENT));
}

extern "C" void
trap_handler (registers regs)
{
  // TODO
  kassert (0);
}
