#include "string.h"
#include <stdbool.h>
#include <stdarg.h>

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

int
strcmp (const char* s1,
	const char* s2)
{
  while (*s1 != 0 && *s2 != 0 && *(s1++) == *(s2++));;
  return *s1 - *s2;
}

size_t
strlen (const char* s)
{
  size_t retval = 0;
  for (; *s != 0; ++retval, ++s) ;;
  return retval;
}

typedef struct {
  char left[1];
  size_t left_start;
  size_t left_size;
  char right[10];
  size_t right_size;
} fbuf_t;  

static void
fbuf_reset (fbuf_t* buf)
{
  buf->left_start = 0;
  buf->left_size = 0;
  buf->right_size = 0;
}

static bool
fbuf_empty_left (const fbuf_t* buf)
{
  return buf->left_start == buf->left_size;
}

static void
fbuf_push_left (fbuf_t* buf,
		char c)
{
  buf->left[buf->left_size++] = c;
}

static char
fbuf_pop_left (fbuf_t* buf)
{
  return buf->left[buf->left_start++];
}

static bool
fbuf_empty_right (const fbuf_t* buf)
{
  return buf->right_size == 0;
}

static void
fbuf_push_right (fbuf_t* buf,
		char c)
{
  buf->right[buf->right_size++] = c;
}

static char
fbuf_pop_right (fbuf_t* buf)
{
  return buf->right[--buf->right_size];
}

typedef struct {
  char* str;
  int count;
  int limit;
} obuf_t;

static void
obuf_init (obuf_t* buf,
	   char* str,
	   size_t size)
{
  buf->str = str;
  buf->count = 0;
  /* Leave space for the terminating 0.*/
  buf->limit = size - 1;
}

static void
obuf_put (obuf_t* buf,
	  char c)
{
  if (buf->count < buf->limit) {
    *buf->str = c;
    ++buf->str;
  }
  ++buf->count;
}

static void
obuf_puts (obuf_t* buf,
	   char* str)
{
  while (*str != 0) {
    obuf_put (buf, *str);
    ++str;
  }
}

int
snprintf (char* str,
	  size_t size,
	  const char* format,
	  ...)
{
  va_list ap;
  fbuf_t fbuf;
  obuf_t obuf;
  char tmp;

  if (size == 0) {
    str = &tmp;
    size = 1;
  }

  va_start (ap, format);
  obuf_init (&obuf, str, size);

  while (*format != 0) {
    switch (*format) {
    case '%':
      ++format;
      if (*format == 0) {
	/* Format error. */
	va_end (ap);
	return -1;
      }
      switch (*format) {
      case 'd':
	{
	  int num = va_arg (ap, int);
	  fbuf_reset (&fbuf);
	  if (num == 0) {
	    obuf_put (&obuf, '0');
	  }
	  else if (num != -2147483648) {
	    if (num < 0) {
	      fbuf_push_left (&fbuf, '-');
	      num *= -1;
	    }

	    while (num != 0) {
	      fbuf_push_right (&fbuf, '0' + num % 10);
	      num /= 10;
	    }

	    while (!fbuf_empty_left (&fbuf)) {
	      obuf_put (&obuf, fbuf_pop_left (&fbuf));
	    }

	    while (!fbuf_empty_right (&fbuf)) {
	      obuf_put (&obuf, fbuf_pop_right (&fbuf));
	    }
	  }
	  else {
	    obuf_puts (&obuf, "-2147483648");
	  }
	  
	  ++format;
	}
	break;
      }
      break;
    default:
      obuf_put (&obuf, *format);
      ++format;
      break;
    }
  }
  
  str[obuf.count] = 0;

  va_end (ap);
  return obuf.count;
}
