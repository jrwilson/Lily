#include "string.h"

char*
strcpy (char* dest,
	const char* src)
{
  char* d = dest;
  while (*src != '\0') {
    *d++ = *src++;
  }
  return dest;
}

char*
strncpy (char* dest,
	 const char* src,
	 size_t n)
{
  char* d = dest;
  size_t c;
  for (c = 0; c != n && *src != '\0'; ++c) {
    *d++ = *src++;
  }
  for (; c != n; ++c) {
    *d++ = '\0';
  }
  return dest;
}

char*
strcat (char* dest,
	const char* src)
{
  char* d = dest;
  while (*d != '\0') {
    ++d;
  }
  while (*src != '\0') {
    *d++ = *src++;
  }
  *d = '\0';

  return dest;
}

char*
strncat (char* dest,
	 const char* src,
	 size_t n)
{
  char* d = dest;
  while (*d != '\0') {
    ++d;
  }
  size_t c;
  for (c = 0; c != n && *src != '\0'; ++c) {
    *d++ = *src++;
  }
  *d = '\0';

  return dest;
}

size_t
strlen (const char* s)
{
  size_t retval = 0;
  while (*s != '\0') {
    ++retval;
    ++s;
  }

  return retval;
}

int
strcmp (const char* s1,
	const char* s2)
{
  while (*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
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
    if (*s1 != *s2 || *s1 == '\0' || *s2 == '\0') {
      return *s1 - *s2;
    }
  }

  return 0;
}

char*
strchr (const char* s,
	int c)
{
  for (; *s != '\0'; ++s) {
    if (*s == c) {
      return (char*)s;
    }
  }

  return NULL;
}

char*
strrchr (const char* s,
	 int c)
{
  char* retval = NULL;
  for (; *s != '\0'; ++s) {
    if (*s == c) {
      retval = (char*)s;
    }
  }
  
  return retval;
}

size_t
strspn (const char* s,
	const char* accept)
{
  size_t retval = 0;
  for (; *s != '\0'; ++s) {
    const char* a = accept;
    while (*a != *s && *a != '\0') {
      ++a;
    }
    if (*a == *s) {
      ++retval;
    }
    else {
      return retval;
    }
  }
  return retval;
}

size_t
strcspn (const char* s,
	 const char* reject)
{
  size_t retval = 0;
  for (; *s != '\0'; ++s) {
    const char* r = reject;
    while (*r != *s && *r != '\0') {
      ++r;
    }
    if (*r == '\0') {
      ++retval;
    }
    else {
      return retval;
    }
  }
  return retval;
}

char*
strpbrk (const char* s,
	 const char* accept)
{
  for (; *s != '\0'; ++s) {
    for (const char* a = accept; *a != '\0'; ++a) {
      if (*a == *s) {
	return (char*)s;
      }
    }
  }

  return NULL;
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
