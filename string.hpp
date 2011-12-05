#ifndef __string_h__
#define __string_h__

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

#include "types.hpp"

void
memset (void* ptr,
	unsigned char value,
	size_t size);

void
memcpy (void* dst,
	const void* src,
	size_t size);

#endif /* __string_h__ */

