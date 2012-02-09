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

bd_t
string_buffer_harvest (string_buffer_t* sb,
		       size_t initial_capacity);

void
string_buffer_put (string_buffer_t* sb,
		   unsigned char c);

size_t
string_buffer_size (const string_buffer_t* sb);

void*
string_buffer_data (const string_buffer_t* sb);

bool
string_buffer_parse (string_buffer_t* sb,
		     bd_t bd,
		     void* ptr,
		     size_t size);

#endif /* STRING_BUFFER_H */
