#include "argv.h"
#include <automaton.h>
#include <string.h>

int
argv_initw (argv_t* a,
	    bd_t* bda,
	    bd_t* bdb)
{
  size_t bd_sz = size_to_pages (sizeof (size_t));
  *bda = buffer_create (bd_sz);
  if (*bda == -1) {
    return -1;
  }
  
  if (buffer_file_open (&a->argv_bf, *bda, true) == -1) {
    buffer_destroy (*bda);
    return -1;
  }
  
  a->count = 0;
  
  if (buffer_file_write (&a->argv_bf, &a->count, sizeof (size_t)) == -1) {
    buffer_destroy (*bda);
    return -1;
  }

  /* Non-empty so it can be mapped. */
  *bdb = buffer_create (1);
  if (*bdb == -1) {
    buffer_destroy (*bda);
    return -1;
  }

  if (buffer_file_open (&a->string_bf, *bdb, true) == -1) {
    buffer_destroy (*bda);
    buffer_destroy (*bdb);
    return -1;
  }

  return 0;
}

int
argv_append (argv_t* a,
	     const char* str)
{
  size_t string_size = strlen (str) + 1;

  /* Get the current offset. */
  size_t string_offset = buffer_file_seek (&a->string_bf, 0, BUFFER_FILE_CURRENT);

  /* Write the string. */
  if (buffer_file_write (&a->string_bf, str, string_size) == -1) {
    return -1;
  }

  /* Write the offset and size. */
  if (buffer_file_write (&a->argv_bf, &string_offset, sizeof (size_t)) == -1 ||
      buffer_file_write (&a->argv_bf, &string_size, sizeof (size_t)) == -1) {
    return -1;
  }

  /* Increment and overwrite the count. */
  ++a->count;

  size_t argv_offset = buffer_file_seek (&a->argv_bf, 0, BUFFER_FILE_CURRENT);
  buffer_file_seek (&a->argv_bf, 0, BUFFER_FILE_SET);
  if (buffer_file_write (&a->argv_bf, &a->count, sizeof (size_t)) == -1) {
    return -1;
  }
  buffer_file_seek (&a->argv_bf, argv_offset, BUFFER_FILE_SET);

  return 0;
}

int
argv_initr (argv_t* a,
	    bd_t bda,
	    bd_t bdb,
	    size_t* count)
{
  if (buffer_file_open (&a->argv_bf, bda, false) == -1) {
    return -1;
  }
  
  if (buffer_file_open (&a->string_bf, bdb, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&a->argv_bf, &a->count, sizeof (size_t)) == -1) {
    return -1;
  }

  /* Read through all of the strings. */
  for (size_t i = 0; i != a->count; ++i) {
    size_t offset;
    size_t size;
    if (buffer_file_read (&a->argv_bf, &offset, sizeof (size_t)) == -1 ||
	buffer_file_read (&a->argv_bf, &size, sizeof (size_t)) == -1) {
      return -1;
    }

    buffer_file_seek (&a->string_bf, offset, BUFFER_FILE_SET);
    const char* str = buffer_file_readp (&a->string_bf, size);
    if (str == 0) {
      return -1;
    }
    if (str[size - 1] != 0) {
      /* Not null-terminated. */
      return -1;
    }
  }

  *count = a->count;

  return 0;
}

const char*
argv_arg (argv_t* a,
	  size_t idx)
{
  if (idx >= a->count) {
    /* Out of range. */
    return 0;
  }

  buffer_file_seek (&a->argv_bf, sizeof (size_t) + idx * (sizeof (size_t) + sizeof (size_t)), BUFFER_FILE_SET);
  size_t offset;
  size_t size;
  if (buffer_file_read (&a->argv_bf, &offset, sizeof (size_t)) == -1 ||
      buffer_file_read (&a->argv_bf, &size, sizeof (size_t)) == -1) {
    return 0;
  }

  buffer_file_seek (&a->string_bf, offset, BUFFER_FILE_SET);
  const char* str = buffer_file_readp (&a->string_bf, size);
  if (str == 0) {
    return 0;
  }
  if (str[size - 1] != 0) {
    /* Not null-terminated. */
    return 0;
  }

  return str;
}
