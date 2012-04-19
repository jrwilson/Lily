#include "buffer_file.h"
#include "automaton.h"
#include "string.h"
#include <stdarg.h>
#include "printf.h"

int
buffer_file_initw (buffer_file_t* bf,
		   lily_error_t* err,
		   bd_t bd)
{
  bf->bd = bd;
  bf->bd_size = buffer_size (err, bd);
  if (bf->bd_size == -1) {
    return -1;
  }
  bf->capacity = bf->bd_size * pagesize ();
  if (bf->bd_size != 0) {
    bf->ptr = buffer_map (err, bd);
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
		   lily_error_t* err,
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
    buffer_unmap (err, bf->bd);
    bf->capacity = ALIGN_UP (new_position, pagesize ());
    bf->bd_size = bf->capacity / pagesize ();
    if (buffer_resize (err, bf->bd, bf->bd_size) != 0) {
      return -1;
    }
    bf->ptr = buffer_map (err, bf->bd);
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
		 lily_error_t* err,
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
    buffer_unmap (err, bf->bd);
    bf->capacity = ALIGN_UP (new_position, pagesize ());
    bf->bd_size = bf->capacity / pagesize ();
    if (buffer_resize (err, bf->bd, bf->bd_size) != 0) {
      return -1;
    }
    bf->ptr = buffer_map (err, bf->bd);
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
		  lily_error_t* err,
		  const char* s)
{
  while (*s != '\0') {
    if (buffer_file_put (bf, err, *s) != 0) {
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
		   lily_error_t* err,
		   bd_t bd)
{
  bf->bd = bd;
  bf->bd_size = buffer_size (err, bd);
  if (bf->bd_size == -1) {
    return -1;
  }
  bf->capacity = bf->bd_size * pagesize ();
  bf->ptr = buffer_map (err, bd);
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
  buffer_file_t* bf;
  lily_error_t* err;
  size_t size;
  bool okay;
} bfprintf_t;

static void
put (void* aux,
     unsigned char c)
{
  bfprintf_t* s = aux;
 
  if (s->okay) {
    s->okay = (buffer_file_put (s->bf, s->err, c) == 0);
  }
  ++s->size;
}

int
bfprintf (buffer_file_t* bf,
	  lily_error_t* err,
	  const char* format,
	  ...)
{
  bfprintf_t s;
  s.bf = bf;
  s.err = err;
  s.size = 0;
  s.okay = true;
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

  return s.size;
}
