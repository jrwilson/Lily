#include "buffer_file.h"
#include "automaton.h"
#include "string.h"

int
buffer_file_open (buffer_file_t* bf,
		  bool can_update,
		  bd_t bd,
		  void* ptr,
		  size_t capacity)
{
  bf->can_update = can_update;
  bf->bd = bd;
  bf->ptr = ptr;
  bf->capacity = capacity;
  bf->position = 0;

  return 0;
}

int
buffer_file_create (buffer_file_t* bf,
		    size_t initial_capacity)
{
  if (initial_capacity == 0) {
    initial_capacity = 1;
  }
  bf->can_update = true;
  bf->bd = buffer_create (1);
  if (bf->bd == -1) {
    return -1;
  }
  bf->ptr = buffer_map (bf->bd);
  if (bf->ptr == 0) {
    return -1;
  }
  bf->capacity = buffer_capacity (bf->bd);
  bf->position = 0;

  return 0;
}

void*
buffer_file_readp (buffer_file_t* bf,
		   size_t size)
{
  if (!bf->can_update) {
    if (bf->position + size <= bf->capacity) {
      /* Success.
	 Return the current position. */
      void* retval = bf->ptr + bf->position;
      /* Advance. */
      bf->position += size;
      return retval;
    }
    else {
      /* Error: Underflow. */
      return 0;
    }
  }
  else {
    /* Error: Cannot form pointers. */
    return 0;
  }
}

int
buffer_file_seek (buffer_file_t* bf,
		  int offset,
		  buffer_file_seek_t s)
{
  switch (s) {
  case BUFFER_FILE_SET:
    if (offset >= 0) {
      bf->position = offset;
      return bf->position;
    }
    else {
      return -1;
    }
    break;
  case BUFFER_FILE_CURRENT:
    {
      int new_position = bf->position + offset;
      if (offset > 0 && new_position < bf->position) {
	/* Overflow. */
	return -1;
      }
      if (offset < 0 && new_position > bf->position) {
	/* Underflow. */
	return -1;
      }
      
      bf->position = new_position;
      return bf->position;
    }
    break;
  }

  return -1;
}

int
buffer_file_write (buffer_file_t* bf,
		   void* ptr,
		   size_t size)
{
  if (!bf->can_update) {
    return -1;
  }

  int new_position = bf->position + size;
  if (new_position < bf->position) {
    return -1;
  }

  /* Resize if necessary. */
  if (bf->capacity < new_position) {
    if (buffer_unmap (bf->bd) == -1) {
      return -1;
    }
    bf->capacity = buffer_resize (bf->bd, new_position);
    if (bf->capacity == -1) {
      return -1;
    }
    bf->ptr = buffer_map (bf->bd);
    if (bf->ptr == 0) {
      return -1;
    }
  }

  memcpy (bf->ptr + bf->position, ptr, size);
  bf->position = new_position;

  return size;
}
