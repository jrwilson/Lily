/*
  File
  ----
  kmain.cpp
  
  Description
  -----------
  Main function of the kernel.

  Authors
  -------
  http://wiki.osdev.org/Bare_bones
  Justin R. Wilson
*/

#include "kassert.hpp"
#include "system_automaton.hpp"

extern "C" void
kmain (void)
{
  system_automaton::run ();
  kassert (0);
}
