#include <stddef.h>
#include <buffer_file.h>
#include <automaton.h>

typedef struct {
  size_t size;
  buffer_file_t bf;
} byte_buffer_t;

int
byte_buffer_initw (byte_buffer_t* bb,
		   bd_t* bda,
		   bd_t* bdb)
{
  bb->size = 0;

  bd_t bd = buffer_create (1);
  if (bd == -1) {
    return -1;
  }

  if (buffer_file_open (&bb->bf, bd, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }
  
  if (buffer_file_write (&bb->bf, &bb->size, sizeof (size_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  *bda = bd;
  *bdb = -1;

  return 0;
}

int
byte_buffer_initr (byte_buffer_t* bb,
		   bd_t bda,
		   bd_t bdb,
		   size_t* size)
{
  if (buffer_file_open (&bb->bf, bda, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&bb->bf, &bb->size, sizeof (size_t)) == -1) {
    return -1;
  }

  *size = bb->size;

  return 0;
}

int
byte_buffer_put (byte_buffer_t* bb,
		 char c)
{
  /* Write the byte. */
  if (buffer_file_write (&bb->bf, &c, 1) == -1) {
    return -1;
  }

  /* Update and overwrite the size. */
  size_t position = buffer_file_seek (&bb->bf, 0, BUFFER_FILE_CURRENT);
  ++bb->size;
  buffer_file_seek (&bb->bf, 0, BUFFER_FILE_SET);
  if (buffer_file_write (&bb->bf, &bb->size, sizeof (size_t)) == -1) {
    return -1;
  }
  buffer_file_seek (&bb->bf, position, BUFFER_FILE_SET);

  return 0;
}

bool
byte_buffer_empty (const byte_buffer_t* bb)
{
  return bb->size == 0;
}

const void*
byte_buffer_readp (byte_buffer_t* bb,
		   size_t size)
{
  return buffer_file_readp (&bb->bf, size);
}
