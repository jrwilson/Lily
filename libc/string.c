#include "string.h"
#include <stdbool.h>

void*
memchr (const void *s,
	int c,
	size_t n)
{
  const unsigned char* data = s;
  while (n != 0) {
    if (*data == c) {
      break;
    }
    --n;
    ++data;
  }

  if (n != 0) {
    return (void*)data;
  }
  else {
    return 0;
  }
}

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

void*
memcpy (void* dest,
	const void* src,
	size_t n)
{
  unsigned char* d = dest;
  const unsigned char* s = src;
  while (n-- != 0) {
    *d++ = *s++;
  }
  return dest;
}

void*
memmove (void* dest,
	 const void* src,
	 size_t n)
{
  if (dest != src) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    
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
  }
  
  return dest;
}

void*
memset (void* s,
	int c,
	size_t n)
{
  
  unsigned char* p = s;
  while (n-- > 0) {
    *p++ = c;
  }
  return s;
}

char*
strchr (const char* s,
	int c)
{
  for (; *s != 0; ++s) {
    if (*s == c) {
      return (char*)s;
    }
  }

  return 0;
}

int
strcmp (const char* s1,
	const char* s2)
{
  while (*s1 == *s2 && *s1 != 0 && *s2 != 0) {
    ++s1;
    ++s2;
  }
  return *s1 - *s2;
}

int
strncmp (const char* s1,
	 const char* s2,
	 size_t n)
{
  for (; n != 0; --n) {
    if (*s1 != *s2 || *s1 == 0 || *s2 == 0) {
      return *s1 - *s2;
    }
  }

  return 0;
}


size_t
strlen (const char* s)
{
  size_t retval = 0;
  for (; *s != 0; ++retval, ++s) ;;
  return retval;
}
