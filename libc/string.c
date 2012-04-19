#include "string.h"
#include "ctype.h"
#include <stdbool.h>
#include <limits.h>
#include "printf.h"

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

static int
toval (char c)
{
  if (isdigit (c)) {
    return c - '0';
  }
  else if (isupper (c)) {
    return c - 'A' + 10;
  }
  else if (islower (c)) {
    return c - 'a' + 10;
  }
  else {
    return -1;
  }
}

static unsigned long int
strtoul_ (string_error_t* err,
	  const char* ptr,
	  const char* nptr,
	  char** endptr,
	  int base,
	  bool negative)
{
  bool once = false;
  unsigned long int retval = 0;
  bool overflow = false;

  while (*ptr != '\0') {
    int val = toval (*ptr);
    if (val == -1 || val >= base) {
      if (!once) {
	/* No digits. */
	if (err != 0) {
	  *err = STRING_INVAL;
	}
	if (endptr != 0) {
	  *endptr = (char*)nptr;
	}
	return 0;
      }
      else {
	break;
      }
    }

    unsigned long int n = retval * base + val;
    if (n < retval) {
      /* Overflow. */
      overflow = true;
    }
    retval = n;
    ++ptr;
    once = true;
  }

  if (!overflow) {
    if (err != 0) {
      *err = STRING_SUCCESS;
    }
    if (endptr != 0) {
      *endptr = (char*)ptr;
    }
    if (negative) {
      return -retval;
    }
    else {
      return retval;
    }
  }
  else {
    if (err != 0) {
      *err = STRING_RANGE;
    }
    if (endptr != 0) {
      *endptr = (char*)ptr;
    }
    return ULONG_MAX;
  }
}

unsigned long int
strtoul (string_error_t* err,
	 const char* nptr,
	 char** endptr,
	 int base)
{
  if (!(base == 0 || (base >= 2 && base <= 36))) {
    /* Bad base. */
    if (err != 0) {
      *err = STRING_INVAL;
    }
    if (endptr != 0) {
      *endptr = (char*)nptr;
    }
    return 0;
  }

  const char* ptr = nptr;

  /* Skip white space. */
  while (*ptr != '\0' && isspace (*ptr)) {
    ++ptr;
  }

  if (*ptr == '\0') {
    /* No digits. */
    if (err != 0) {
      *err = STRING_INVAL;
    }
    if (endptr != 0) {
      *endptr = (char*)nptr;
    }
    return 0;
  }

  bool negative = false;
  if (*ptr == '+') {
    ++ptr;
  }
  else if (*ptr == '-') {
    ++ptr;
    negative = true;
  }

  if (*ptr == '\0') {
    /* No digits. */
    if (err != 0) {
      *err = STRING_INVAL;
    }
    if (endptr != 0) {
      *endptr = (char*)nptr;
    }
    return 0;
  }

  if (base == 0 || base == 8 || base == 16) {
    if (*ptr == '0') {
      ++ptr;
      if (*ptr == '\0') {
	/* Zero. */
	if (err != 0) {
	  *err = STRING_SUCCESS;
	}
	if (endptr != 0) {
	  *endptr = (char*)ptr;
	}
	return 0;
      }
      else if (*ptr == 'x') {
	++ptr;
	/* Number is hex. */
	return strtoul_ (err, ptr, nptr, endptr, 16, negative);
      }
      else {
	/* Number is octal. */
	return strtoul_ (err, ptr, nptr, endptr, 8, negative);
      }
    }
    else {
      /* Number is decimal. */
      return strtoul_ (err, ptr, nptr, endptr, 10, negative);
    }
  }
  else {
    return strtoul_ (err, ptr, nptr, endptr, base, negative);
  }
}

typedef struct {
  char* buffer;
  size_t size;
  size_t capacity;
} snprintf_t;

static void
put (void* aux,
     unsigned char c)
{
  snprintf_t* s = aux;
 
  if (s->size < s->capacity) {
    s->buffer[s->size] = c;
  }
  s->size++;
}

int
snprintf (char* str,
	  size_t size,
	  const char* format,
	  ...)
{
  snprintf_t s;
  s.buffer = str;
  s.size = 0;
  s.capacity = size;
  printf_t p;
  p.aux = &s;
  p.put = put;

  va_list args;
  va_start (args, format);
  int retval = printf (&p, format, args);
  va_end (args);

  if (retval != 0) {
    return -1;
  }

  retval = s.size;

  if (s.size < s.capacity) {
    put (&s, '\0');
  }
  else {
    str[size - 1] = '\0';
  }

  return retval;
}
