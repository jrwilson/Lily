#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <lily/types.h>

typedef struct {
  size_t size;
  size_t capacity;
  ptrdiff_t data; /* Relative offset. */
} string_t;

typedef struct {
  bd_t bd;		/* Buffer descriptor. */
  string_t* string;	/* A string_t mapped over bd. */
  unsigned char* data;	/* The data of the string.  */
} string_buffer_t;

void
string_buffer_init (string_buffer_t* sb,
		    size_t initial_capacity);

void
string_buffer_reserve (string_buffer_t* sb,
		       size_t new_capacity);

void
string_buffer_putc (string_buffer_t* sb,
		    char c);

void
string_buffer_puts (string_buffer_t* sb,
		    const char* s);

void
string_buffer_append (string_buffer_t* sb,
		      const void* ptr,
		      size_t size);

bd_t
string_buffer_bd (const string_buffer_t* sb);

size_t
string_buffer_size (const string_buffer_t* sb);

void*
string_buffer_data (const string_buffer_t* sb);

bool
string_buffer_parse (string_buffer_t* sb,
		     bd_t bd,
		     void* ptr,
		     size_t size);

int
sbprintf (string_buffer_t* sb,
	  const char* format,
	  ...);

#endif /* STRING_BUFFER_H */
