#ifndef BUFFER_FILE_H
#define BUFFER_FILE_H

#include <stddef.h>
#include <stdbool.h>
#include <lily/types.h>

/* Buffer File
   ===========
   Treat a buffer like a file.

   The size of the file is stored in the first sizeof (size_t) bytes of the buffer.
   The adjust for the header is made on entry and exit.
   Internally, the sizes, capacities, position, etc. do not account for it.
*/

typedef struct {
  bd_t bd;
  size_t bd_size;	/* Number of pages in the buffer. */
  size_t capacity;	/* Capacity of the buffer in bytes. capacity = bd_size * pagesize (). */
  void* ptr;		/* The data in the buffer. */
  size_t size;		/* Number of bytes written to the buffer.  size <= capacity. */
  size_t position;	/* The position of the next read or write.  position <= size. */
  bool can_update;	/* Flag indicating that the buffer can be modified. */
} buffer_file_t;

int
buffer_file_initc (buffer_file_t* bf,
		   bd_t bd);

int
buffer_file_write (buffer_file_t* bf,
		   const void* ptr,
		   size_t size);

int
buffer_file_put (buffer_file_t* bf,
		 char c);

int
buffer_file_initr (buffer_file_t* bf,
		   bd_t bd);

const void*
buffer_file_readp (buffer_file_t* bf,
		   size_t size);

int
buffer_file_read (buffer_file_t* bf,
		  void* ptr,
		  size_t size);

bd_t
buffer_file_bd (const buffer_file_t* bf);

size_t
buffer_file_size (const buffer_file_t* bf);

size_t
buffer_file_position (const buffer_file_t* bf);

int
buffer_file_seek (buffer_file_t* bf,
		  size_t position);

#endif /* BUFFER_FILE_H */
