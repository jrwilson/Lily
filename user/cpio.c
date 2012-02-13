#include "cpio.h"
#include <string.h>
#include <dymem.h>

static const char*
align_up (const char* address,
	  unsigned int radix)
{
  return (const char*) (((size_t)address + radix - 1) & ~(radix - 1));
}

static unsigned int
from_hex (const char* s)
{
  unsigned int retval = 0;

  for (int idx = 0; idx < 8; ++idx) {
    retval <<= 4;
    if (s[idx] >= '0' && s[idx] <= '9') {
      retval |= s[idx] - '0';
    }
    else if (s[idx] >= 'A' && s[idx] <= 'F') {
      retval |= s[idx] - 'A' + 10;
    }
  }

  return retval;
}

typedef struct {
  char magic[6];
  char inode[8];
  char mode[8];
  char uid[8];
  char gid[8];
  char nlink[8];
  char mtime[8];
  char filesize[8];
  char dev_major[8];
  char dev_minor[8];
  char rdev_major[8];
  char rdev_minor[8];
  char namesize[8];
  char checksum[8];
} cpio_header_t;

cpio_file_t*
parse_cpio (const char** begin_ptr,
	    const char* end)
{
  const char* begin = *begin_ptr;
  if (begin >= end) {
    return 0;
  }

  /* Align to a 4-byte boundary. */
  begin = align_up (begin, 4);

  if (begin + sizeof (cpio_header_t) > end) {
    /* Underflow. */
    return 0;
  }
  cpio_header_t* h = (cpio_header_t*)begin;

  if (memcmp (h->magic, "070701", 6) == 0) {

  }
  else if (memcmp (h->magic, "070702", 6) == 0) {

  }
  else {
    /* Bad magic number. */
    return 0;
  }
  size_t filesize = from_hex (h->filesize);
  size_t namesize = from_hex (h->namesize);
  begin += sizeof (cpio_header_t);

  if (begin + namesize > end) {
    /* Underflow. */
    return 0;
  }
  if (begin[namesize - 1] != 0) {
    /* Name is not null terminated. */
    return 0;
  }
  const char* name = begin;
  begin += namesize;

  /* Align to a 4-byte boundary. */
  begin = align_up (begin, 4);

  if (begin + filesize > end) {
    /* Underflow. */
    return 0;
  }
  const char* data = begin;
  begin += filesize;

  /* Check for the trailer. */
  if (strcmp (name, "TRAILER!!!") == 0) {
    /* Done. */
    return 0;
  }

  /* Create a new entry. */
  cpio_file_t* f = malloc (sizeof (cpio_file_t));
  /* Record the name. */
  f->name = malloc (namesize);
  memcpy (f->name, name, namesize);
  f->name_size = namesize;
  /* Create a buffer and copy the file content. */
  f->buffer = buffer_create (filesize);
  memcpy (buffer_map (f->buffer), data, filesize);
  buffer_unmap (f->buffer);
  f->buffer_size = filesize;

  *begin_ptr = begin;
  return f;
}

void
cpio_file_destroy (cpio_file_t* file)
{
  buffer_destroy (file->buffer);
  free (file->name);
  free (file);
}
