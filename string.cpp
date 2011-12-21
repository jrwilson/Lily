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

#include <string.h>

size_t
strlen (const char* p)
{
  size_t retval = 0;
  while (*p != 0) {
    ++retval;
    ++p;
  }
  return retval;
}

int
strcmp (const char* p,
	const char* q)
{
  while (*p != 0 && *q != 0 && *p++ == *q++);;

  return *p - *q;
}
void*
memset (void* ptr,
	int value,
	size_t size)
{
  unsigned char* p = static_cast<unsigned char*> (ptr);
  while (size-- > 0) {
    *p++ = value;
  }
  return ptr;
}

void*
memcpy (void* dst,
	const void* src,
	size_t size)
{
  unsigned char* d = static_cast<unsigned char*> (dst);
  const unsigned char* s = static_cast<const unsigned char*> (src);
  while (size-- > 0) {
    *d++ = *s++;
  }
  return dst;
}
