#include "cpio.h"
#include <string.h>
#include <dymem.h>

static int
align_up (int address,
	  unsigned int radix)
{
  return ((size_t)address + radix - 1) & ~(radix - 1);
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
parse_cpio (buffer_file_t* bf)
{
  /* Align to a 4-byte boundary. */
  int pos = buffer_file_seek (bf, 0, BUFFER_FILE_CURRENT);
  pos = align_up (pos, 4);
  buffer_file_seek (bf, pos, BUFFER_FILE_SET);

  const cpio_header_t* h = buffer_file_readp (bf, sizeof (cpio_header_t));
  if (h == 0) {
    /* Underflow. */
    return 0;
  }

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

  const char* name = buffer_file_readp (bf, namesize);
  if (name == 0) {
    return 0;
  }
  if (name[namesize - 1] != 0) {
    /* Name is not null terminated. */
    return 0;
  }

  /* Align to a 4-byte boundary. */
  pos = buffer_file_seek (bf, 0, BUFFER_FILE_CURRENT);
  pos = align_up (pos, 4);
  buffer_file_seek (bf, pos, BUFFER_FILE_SET);

  const char* data = buffer_file_readp (bf, filesize);
  if (data == 0) {
    return 0;
  }

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
  f->mode = from_hex (h->mode);
  /* Create a buffer and copy the file content. */
  f->bd_size = ALIGN_UP (filesize, pagesize ()) / pagesize ();
  f->bd = buffer_create (f->bd_size);
  memcpy (buffer_map (f->bd), data, filesize);
  buffer_unmap (f->bd);
  f->size = filesize;

  return f;
}

void
cpio_file_destroy (cpio_file_t* file)
{
  buffer_destroy (file->bd);
  free (file->name);
  free (file);
}
