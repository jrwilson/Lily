/*
  File
  ----
  string.hpp
  
  Description
  -----------
  Functions for working with strings.

  Authors:
  Justin R. Wilson
*/

#include "string.hpp"

void
memset (void* ptr,
	unsigned char value,
	size_t size)
{
  unsigned char* p = static_cast<unsigned char*> (ptr);
  while (size > 0) {
    *p = value;
    ++p;
    --size;
  }
}
