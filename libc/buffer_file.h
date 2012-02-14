#ifndef BUFFER_FILE_H
#define BUFFER_FILE_h

#include <stddef.h>
#include <stdbool.h>
#include <lily/types.h>

typedef enum {
  BUFFER_FILE_SET,
  BUFFER_FILE_CURRENT,
} buffer_file_seek_t;

typedef struct {
  bool can_update;
  bd_t bd;
  void* ptr;
  size_t capacity;
  int position;
} buffer_file_t;

int
buffer_file_open (buffer_file_t* bf,
		  bool can_update,
		  bd_t bd,
		  void* ptr,
		  size_t capacity);

int
buffer_file_create (buffer_file_t* bf,
		    size_t initial_capacity);

const void*
buffer_file_readp (buffer_file_t* bf,
		   size_t size);

int
buffer_file_seek (buffer_file_t* bf,
		  int offset,
		  buffer_file_seek_t s);

int
buffer_file_write (buffer_file_t* bf,
		   const void* ptr,
		   size_t size);

bd_t
buffer_file_bd (const buffer_file_t* bf);

#endif /* BUFFER_FILE_H */
