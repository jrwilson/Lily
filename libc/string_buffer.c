#include "string_buffer.h"
#include "automaton.h"
#include "buffer_heap.h"
#include <stdarg.h>
#include "string.h"

void
string_buffer_init (string_buffer_t* sb,
		    size_t initial_capacity)
{
  sb->bd = buffer_create (sizeof (string_t) + initial_capacity);
  size_t real_capacity = buffer_capacity (sb->bd);
  void* ptr = buffer_map (sb->bd);
  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, real_capacity);
  sb->string = buffer_heap_alloc (&heap, sizeof (string_t));
  sb->data = buffer_heap_alloc (&heap, real_capacity - sizeof (string_t));
  
  sb->string->size = 0;
  sb->string->capacity = real_capacity - sizeof (string_t);
  sb->string->data = (void*)sb->data - (void*)&sb->string->data;
}

void
string_buffer_reserve (string_buffer_t* sb,
		       size_t new_capacity)
{
  if (new_capacity > sb->string->capacity) {
    /* Double in capacity. */
    buffer_unmap (sb->bd);
    size_t real_capacity = buffer_resize (sb->bd, new_capacity);
    void* ptr = buffer_map (sb->bd);
    buffer_heap_t heap;
    buffer_heap_init (&heap, ptr, real_capacity);
    sb->string = buffer_heap_alloc (&heap, sizeof (string_t));
    sb->data = buffer_heap_alloc (&heap, real_capacity - sizeof (string_t));

    /* sb->string->size has not changed. */
    sb->string->capacity = real_capacity - sizeof (string_t);
    /* sb->string->data has not changed. */
  }
}

void
string_buffer_putc (string_buffer_t* sb,
		    char c)
{
  if (sb->string->size == sb->string->capacity) {
    /* Double in capacity. */
    string_buffer_reserve (sb, sb->string->capacity);
  }
  
  /* Store the scan code. */
  sb->data[sb->string->size++] = c;
}

void
string_buffer_puts (string_buffer_t* sb,
		    const char* s)
{
  while (*s != 0) {
    string_buffer_putc (sb, *s++);
  }
}

void
string_buffer_append (string_buffer_t* sb,
		      const void* ptr,
		      size_t size)
{
  string_buffer_reserve (sb, size);
  memcpy (&sb->data[sb->string->size], ptr, size);
  sb->string->size += size;
}

bd_t
string_buffer_bd (const string_buffer_t* sb)
{
  return sb->bd;
}

size_t
string_buffer_size (const string_buffer_t* sb)
{
  return sb->string->size;
}

void*
string_buffer_data (const string_buffer_t* sb)
{
  return sb->data;
}

bool
string_buffer_parse (string_buffer_t* sb,
		     bd_t bd,
		     void* ptr,
		     size_t size)
{
  sb->bd = bd;
  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, size);
  sb->string = buffer_heap_begin (&heap);
  if (buffer_heap_check (&heap, sb->string, sizeof (string_t))) {
    sb->data = (void*)&sb->string->data + sb->string->data;
    if (buffer_heap_check (&heap, sb->data, sb->string->capacity)) {
      return true;
    }
  }

  return false;
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
sbprintf (string_buffer_t* sb,
	  const char* format,
	  ...)
{
  va_list ap;
  fbuf_t fbuf;
  size_t size_before = string_buffer_size (sb);

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
	    string_buffer_putc (sb, '0');
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
	      string_buffer_putc (sb, fbuf_pop_left (&fbuf));
	    }

	    while (!fbuf_empty_right (&fbuf)) {
	      string_buffer_putc (sb, fbuf_pop_right (&fbuf));
	    }
	  }
	  else {
	    string_buffer_puts (sb, "-2147483648");
	  }
	  
	  ++format;
	}
	break;
      case 'x':
	{
	  int num = va_arg (ap, int);
	  fbuf_reset (&fbuf);
	  if (num == 0) {
	    string_buffer_putc (sb, '0');
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
	      string_buffer_putc (sb, fbuf_pop_left (&fbuf));
	    }

	    while (!fbuf_empty_right (&fbuf)) {
	      string_buffer_putc (sb, fbuf_pop_right (&fbuf));
	    }
	  }
	  else {
	    string_buffer_puts (sb, "-ffffffff");
	  }
	  
	  ++format;
	}
	break;
      case 's':
	{
	  char* str = va_arg (ap, char*);
	  string_buffer_puts (sb, str);
	  ++format;
	}
	break;
      }
      break;
    default:
      string_buffer_putc (sb, *format);
      ++format;
      break;
    }
  }
  
  va_end (ap);
  return string_buffer_size (sb) - size_before;
}
