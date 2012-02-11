#ifndef __string_hpp__
#define __string_hpp__

#include <stddef.h>

// TODO:  Provide optimized versions.

inline int
memcmp (const void* s1,
	const void* s2,
	size_t n)
{
  const char* r = static_cast<const char*> (s1);
  const char* s = static_cast<const char*> (s2);
  
  for (; n != 0; --n, ++r, ++s) {
    if (*r != *s) {
      return *r - *s;
    }
  }
  
  return 0;
}

inline void*
memcpy (void* dest,
	const void* src,
	size_t n)
{
  unsigned char* d = static_cast<unsigned char*> (dest);
  const unsigned char* s = static_cast<const unsigned char*> (src);
  while (n-- != 0) {
    *d++ = *s++;
  }
  return dest;
}

inline void*
memmove (void* dest,
	 const void* src,
	 size_t n)
{
  unsigned char* d = static_cast<unsigned char*> (dest);
  const unsigned char* s = static_cast<const unsigned char*> (src);
  
  if (s >= d || s + n <= d) {
    // No overlap.
    memcpy (dest, src, n);
  }
  else {
    s += n;
    d += n;
    while (n-- != 0) {
      *(--d) = *(--s);
    }
  }
  
  return dest;
}

inline void*
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

inline int
strcmp (const char* s1,
	const char* s2)
{
  while (*s1 != 0 && *s2 != 0) {
    if (*s1 != *s2) {
      break;
    }
    ++s1;
    ++s2;
  }
  
  return *s1 - *s2;
}

inline int
strncmp (const char* s1,
	 const char* s2,
	 size_t n)
{
  while (n != 0 && *s1 != 0 && *s2 != 0 && *s1 == *s2) {
    ++s1;
    ++s2;
    --n;
  }
    
  return (n == 0) ? 0 : *s1 - *s2;
}

inline size_t
strlen (const char* s)
{
  size_t retval = 0;
  for (; *s != 0; ++retval, ++s) ;;
  return retval;
}

#endif /* __string_hpp__ */
