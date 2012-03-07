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
  
  if (buffer_file_initw (&a->argv_bf, *bda) == -1) {
    buffer_destroy (*bda);
    return -1;
  }
  
  a->argc = 0;
  
  if (buffer_file_write (&a->argv_bf, &a->argc, sizeof (size_t)) == -1) {
    buffer_destroy (*bda);
    return -1;
  }

  /* Non-empty so it can be mapped. */
  *bdb = buffer_create (1);
  if (*bdb == -1) {
    buffer_destroy (*bda);
    return -1;
  }

  if (buffer_file_initw (&a->string_bf, *bdb) == -1) {
    buffer_destroy (*bda);
    buffer_destroy (*bdb);
    return -1;
  }

  return 0;
}

int
argv_append (argv_t* a,
	     const void* ptr,
	     size_t size)
{
  /* Get the current offset. */
  size_t string_offset = buffer_file_position (&a->string_bf);

  /* Write the string. */
  if (buffer_file_write (&a->string_bf, ptr, size) == -1) {
    return -1;
  }

  /* Write the offset and size. */
  if (buffer_file_write (&a->argv_bf, &string_offset, sizeof (size_t)) == -1 ||
      buffer_file_write (&a->argv_bf, &size, sizeof (size_t)) == -1) {
    return -1;
  }

  /* Increment and overwrite the count. */
  ++a->argc;

  size_t argv_offset = buffer_file_position (&a->argv_bf);
  if (buffer_file_seek (&a->argv_bf, 0) == -1) {
    return -1;
  }
  if (buffer_file_write (&a->argv_bf, &a->argc, sizeof (size_t)) == -1) {
    return -1;
  }
  if (buffer_file_seek (&a->argv_bf, argv_offset) == -1) {
    return -1;
  }

  return 0;
}

int
argv_initr (argv_t* a,
	    bd_t bda,
	    bd_t bdb,
	    size_t* argc)
{
  if (buffer_file_initr (&a->argv_bf, bda) == -1) {
    return -1;
  }
  
  if (buffer_file_initr (&a->string_bf, bdb) == -1) {
    return -1;
  }

  if (buffer_file_read (&a->argv_bf, &a->argc, sizeof (size_t)) == -1) {
    return -1;
  }

  /* Read through all of the strings. */
  for (size_t i = 0; i != a->argc; ++i) {
    size_t offset;
    size_t size;
    if (buffer_file_read (&a->argv_bf, &offset, sizeof (size_t)) == -1 ||
	buffer_file_read (&a->argv_bf, &size, sizeof (size_t)) == -1) {
      return -1;
    }

    if (buffer_file_seek (&a->string_bf, offset) == -1) {
      return -1;
    }

    const char* str = buffer_file_readp (&a->string_bf, size);
    if (str == 0) {
      return -1;
    }
    if (str[size - 1] != 0) {
      /* Not null-terminated. */
      return -1;
    }
  }

  *argc = a->argc;

  return 0;
}

int
argv_arg (argv_t* a,
	  size_t idx,
	  const void** ptr,
	  size_t* size)
{
  if (idx >= a->argc) {
    /* Out of range. */
    return -1;
  }

  if (buffer_file_seek (&a->argv_bf, sizeof (size_t) + idx * (sizeof (size_t) + sizeof (size_t))) == -1) {
    return -1;
  }

  size_t offset;
  if (buffer_file_read (&a->argv_bf, &offset, sizeof (size_t)) == -1 ||
      buffer_file_read (&a->argv_bf, size, sizeof (size_t)) == -1) {
    return -1;
  }

  if (buffer_file_seek (&a->string_bf, offset) == -1) {
    return -1;
  }

  *ptr = buffer_file_readp (&a->string_bf, *size);
  if (*ptr == 0) {
    return -1;
  }

  return 0;
}
