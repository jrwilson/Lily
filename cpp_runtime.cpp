/*
  File
  ----
  cpp_runtime.cpp
  
  Description
  -----------
  Functions to support the C++ run-time.

  Authors:
  Justin R. Wilson
*/

#include "kassert.hpp"

extern "C" void
__cxa_pure_virtual (void)
{
  kputs ("Pure virtual function called\n");
  halt ();
}
