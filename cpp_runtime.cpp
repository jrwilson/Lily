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

void *__dso_handle;

extern "C" void
__cxa_pure_virtual (void)
{
  kout << "Pure virtual function called" << endl;
  halt ();
}

extern "C" int
__cxa_atexit (void (* /*destructor*/)(void*),
	      void* /* arg */,
	      void* /* dso */)
{
  // Do nothing successfully.
  return 0;
}
