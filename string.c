/*
  File
  ----
  string.h
  
  Description
  -----------
  Functions for working with strings.

  Authors:
  Justin R. Wilson
*/

#include "string.h"

void
memset (void* ptr,
	unsigned char value,
	size_t size)
{
  unsigned char* p = ptr;
  while (size > 0) {
    *p = value;
    ++p;
    --size;
  }
}
