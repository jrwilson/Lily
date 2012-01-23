#include "string.h"

int
memcmp (const void* s1,
	const void* s2,
	size_t n)
{
  const char* r = s1;
  const char* s = s2;
  
  for (; n != 0; --n, ++r, ++s) {
    if (*r != *s) {
      return *r - *s;
    }
  }
  
  return 0;
}

int
strcmp (const char* s1,
	const char* s2)
{
  while (*s1 != 0 && *s2 != 0 && *(s1++) == *(s2++));;
  return *s1 - *s2;
}
