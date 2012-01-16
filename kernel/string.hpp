#ifndef __string_hpp__
#define __string_hpp__

#include <stddef.h>

// TODO:  Provide optimized versions.

static inline void*
memset (void* s,
	int c,
	size_t n)
{

  unsigned char* p = static_cast<unsigned char*> (s);
  while (n-- > 0) {
    *p++ = c;
  }
  return s;
}

static inline int
strcmp (const char* s1,
	const char* s2)
{
  while (*s1 != 0 && *s2 != 0 && *(s1++) == *(s2++));;
  
  return *s1 - *s2;
}

static inline size_t
strlen (const char* s)
{
  size_t retval = 0;
  for (; *s != 0; ++retval, ++s) ;;
  return retval;
}

#endif /* __string_hpp__ */
