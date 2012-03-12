#include "buffer_file.h"
#include "automaton.h"
#include "string.h"
#include <stdarg.h>

int
buffer_file_initw (buffer_file_t* bf,
		   bd_t bd)
{
  bf->bd = bd;
  bf->bd_size = buffer_size (bd);
  if (bf->bd_size == -1) {
    return -1;
  }
  bf->capacity = bf->bd_size * pagesize ();
  if (bf->bd_size != 0) {
    bf->ptr = buffer_map (bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }
  else {
    bf->ptr = 0;
  }
  bf->size = sizeof (size_t);
  bf->position = sizeof (size_t);
  bf->can_update = true;

  return 0;
}

int
buffer_file_write (buffer_file_t* bf,
		   const void* ptr,
		   size_t size)
{
  if (!bf->can_update) {
    return -1;
  }

  size_t new_position = bf->position + size;
  if (new_position < bf->position) {
    /* Overflow. */
    return -1;
  }

  /* Resize if necessary. */
  if (bf->capacity < new_position) {
    buffer_unmap (bf->bd);
    bf->capacity = ALIGN_UP (new_position, pagesize ());
    bf->bd_size = bf->capacity / pagesize ();
    if (buffer_resize (bf->bd, bf->bd_size) == -1) {
      return -1;
    }
    bf->ptr = buffer_map (bf->bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }

  memcpy (bf->ptr + bf->position, ptr, size);
  bf->position = new_position;
  if (bf->position > bf->size) {
    bf->size = bf->position;
    *((size_t*)bf->ptr) = bf->size;
  }

  return 0;
}

int
buffer_file_put (buffer_file_t* bf,
		 char c)
{
  if (!bf->can_update) {
    return -1;
  }
  
  size_t new_position = bf->position + 1;
  if (new_position < bf->position) {
    /* Overflow. */
    return -1;
  }
  
  /* Resize if necessary. */
  if (bf->capacity < new_position) {
    buffer_unmap (bf->bd);
    bf->capacity = ALIGN_UP (new_position, pagesize ());
    bf->bd_size = bf->capacity / pagesize ();
    if (buffer_resize (bf->bd, bf->bd_size) == -1) {
      return -1;
    }
    bf->ptr = buffer_map (bf->bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }
  
  *((char*)(bf->ptr + bf->position)) = c;
  bf->position = new_position;
  if (bf->position > bf->size) {
    bf->size = bf->position;
    *((size_t*)bf->ptr) = bf->size;
  }

  return 0;
}

int
buffer_file_puts (buffer_file_t* bf,
		  const char* s)
{
  size_t size = strlen (s);
  const char* end = s + size;
  while (s != end) {
    if (buffer_file_put (bf, *s) == -1) {
      return -1;
    }
    ++s;
  }

  return 0;
}

void
buffer_file_truncate (buffer_file_t* bf)
{
  bf->position = sizeof (size_t);
  bf->size = sizeof (size_t);
}

int
buffer_file_initr (buffer_file_t* bf,
		   bd_t bd)
{
  bf->bd = bd;
  bf->bd_size = buffer_size (bd);
  if (bf->bd_size == -1) {
    return -1;
  }
  bf->capacity = bf->bd_size * pagesize ();
  bf->ptr = buffer_map (bd);
  if (bf->ptr == 0) {
    return -1;
  }
  bf->size = *((size_t*)bf->ptr);
  bf->position = sizeof (size_t);
  bf->can_update = false;

  return 0;
}

const void*
buffer_file_readp (buffer_file_t* bf,
		   size_t size)
{
  if (bf->can_update) {
    /* Error: Cannot form pointers. */
    return 0;
  }

  if (bf->position > bf->size) {
    return 0;
  }

  if (size > (bf->size - bf->position)) {
    /* Error: Not enough data. */
    return 0;
  }
  
  /* Success.
     Return the current position. */
  void* retval = bf->ptr + bf->position;
  /* Advance. */
  bf->position += size;
  return retval;
}

int
buffer_file_read (buffer_file_t* bf,
		  void* ptr,
		  size_t size)
{
  if (bf->position > bf->size) {
    return -1;
  }

  if (size > (bf->size - bf->position)) {
    /* Error: Not enough data. */
    return -1;
  }

  memcpy (ptr, bf->ptr + bf->position, size);
  bf->position += size;

  return 0;
}

bd_t
buffer_file_bd (const buffer_file_t* bf)
{
  return bf->bd;
}

size_t
buffer_file_size (const buffer_file_t* bf)
{
  return bf->size - sizeof (size_t);
}

size_t
buffer_file_position (const buffer_file_t* bf)
{
  return bf->position - sizeof (size_t);
}

int
buffer_file_seek (buffer_file_t* bf,
		  size_t position)
{
  position += sizeof (size_t);

  if (position < sizeof (size_t)) {
    return -1;
  }

  bf->position = position;
  return 0;
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

static char
to_hex (int x)
{
  if (x < 10) {
    return x + '0';
  }
  else {
    return x - 10 + 'a';
  }
}

int
bfprintf (buffer_file_t* bf,
	  const char* format,
	  ...)
{
  va_list ap;
  fbuf_t fbuf;

  va_start (ap, format);

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
	    if (buffer_file_put (bf, '0') == -1) {
	      return -1;
	    }
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
	      if (buffer_file_put (bf, fbuf_pop_left (&fbuf)) == -1) {
		return -1;
	      }
	    }

	    while (!fbuf_empty_right (&fbuf)) {
	      if (buffer_file_put (bf, fbuf_pop_right (&fbuf)) == -1) {
		return -1;
	      }
	    }
	  }
	  else {
	    if (buffer_file_puts (bf, "-2147483648") == -1) {
	      return -1;
	    }
	  }
	  
	  ++format;
	}
	break;
      case 'x':
	{
	  int num = va_arg (ap, int);
	  fbuf_reset (&fbuf);
	  if (num == 0) {
	    if (buffer_file_put (bf, '0') == -1) {
	      return -1;
	    }
	  }
	  else if (num != -2147483648) {
	    if (num < 0) {
	      fbuf_push_left (&fbuf, '-');
	      num *= -1;
	    }

	    while (num != 0) {
	      fbuf_push_right (&fbuf, to_hex (num % 16));
	      num /= 16;
	    }

	    while (!fbuf_empty_left (&fbuf)) {
	      if (buffer_file_put (bf, fbuf_pop_left (&fbuf)) == -1) {
		return -1;
	      }
	    }

	    while (!fbuf_empty_right (&fbuf)) {
	      if (buffer_file_put (bf, fbuf_pop_right (&fbuf)) == -1) {
		return -1;
	      }
	    }
	  }
	  else {
	    if (buffer_file_puts (bf, "-ffffffff") == -1) {
	      return -1;
	    }
	  }
	  
	  ++format;
	}
	break;
      case 's':
	{
	  char* str = va_arg (ap, char*);
	  if (buffer_file_puts (bf, str) == -1) {
	    return -1;
	  }
	  ++format;
	}
	break;
      }
      break;
    default:
      if (buffer_file_put (bf, *format) == -1) {
	return -1;
      }
      ++format;
      break;
    }
  }
  
  va_end (ap);
  return 0;
}
