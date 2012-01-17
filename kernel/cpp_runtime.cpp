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
#include "kernel_alloc.hpp"

void*
operator new (size_t sz)
{
  return kernel_alloc::alloc (sz);
}

void*
operator new[] (size_t sz)
{
  return kernel_alloc::alloc (sz);
}

void
operator delete (void* ptr)
{
  kernel_alloc::free (ptr);
}

void
operator delete[] (void* ptr)
{
  kernel_alloc::free (ptr);
}

void *__dso_handle;

extern "C" int
__cxa_atexit (void (* /*destructor*/)(void*),
	      void* /* arg */,
	      void* /* dso */)
{
  // Do nothing successfully.
  return 0;
}
