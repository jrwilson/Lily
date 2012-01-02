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

#include <stddef.h>

namespace ltl {
  
  inline char*
  strcpy (char* p,
	  const char* q)
  {
    char* retval = p;
    while (*q != 0) {
      *p++ = *q++;
    }
    return retval;
  }
  
  inline size_t
  strlen (const char* p)
  {
    size_t retval = 0;
    while (*p != 0) {
      ++retval;
      ++p;
    }
    return retval;
  }
  
  inline int
  strcmp (const char* p,
	  const char* q)
  {
    while (*p != 0 && *q != 0 && *(p++) == *(q++));;
    
    return *p - *q;
  }
  
  inline int
  strncmp (const char* p,
	   const char* q,
	   int n)
  {
    while (n != 0 && *p != 0 && *q != 0 && *p == *q) {
      ++p;
      ++q;
      --n;
    }
    
    return (n == 0) ? 0 : *p - *q;
  }
  
  inline void*
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
  
  inline int
  memcmp (const void* p,
	  const void* q,
	  size_t n)
  {
    const char* r = static_cast<const char*> (p);
    const char* s = static_cast<const char*> (q);
    
    for (; n != 0; --n, ++r, ++s) {
      if (*r != *s) {
	return *r - *s;
      }
    }
    
    return 0;
  }
  
  inline void*
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
  
}

#endif /* __string_h__ */

