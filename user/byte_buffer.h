#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <buffer_file.h>

typedef struct {
  size_t size;
  buffer_file_t bf;
} byte_buffer_t;

int
byte_buffer_initw (byte_buffer_t* bb,
		   bd_t* bda,
		   bd_t* bdb);

int
byte_buffer_initr (byte_buffer_t* bb,
		   bd_t bda,
		   bd_t bdb,
		   size_t* size);

int
byte_buffer_put (byte_buffer_t* bb,
		 char c);

bool
byte_buffer_empty (const byte_buffer_t* bb);

const void*
byte_buffer_readp (byte_buffer_t* bb,
		   size_t size);

#endif /* BYTE_BUFFER_H */
