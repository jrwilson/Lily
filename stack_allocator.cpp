/*
  File
  ----
  stack_allocator.hpp
  
  Description
  -----------
  Allocate frames in a region of memory using a stack.

  Authors:
  Justin R. Wilson
*/

#include "stack_allocator.hpp"

const size_t stack_allocator::MAX_REGION_SIZE = 0x07FFF000;
