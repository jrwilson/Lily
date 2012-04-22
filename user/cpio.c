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

int
cpio_archive_init (cpio_archive_t* ar,
		   bd_t bd)
{
  if (buffer_file_initr (&ar->bf, bd) != 0) {
    return -1;
  }

  return 0;
}

cpio_file_t*
cpio_archive_next_file (cpio_archive_t* ar)
{
  /* Align to a 4-byte boundary. */
  size_t pos = buffer_file_position (&ar->bf);
  pos = align_up (pos, 4);
  if (buffer_file_seek (&ar->bf, pos) != 0) {
    return 0;
  }

  const cpio_header_t* h = buffer_file_readp (&ar->bf, sizeof (cpio_header_t));
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

  const char* name = buffer_file_readp (&ar->bf, namesize);
  if (name == 0) {
    return 0;
  }
  if (name[namesize - 1] != 0) {
    /* Name is not null terminated. */
    return 0;
  }

  /* Align to a 4-byte boundary. */
  pos = buffer_file_position (&ar->bf);
  pos = align_up (pos, 4);
  if (buffer_file_seek (&ar->bf, pos) != 0) {
    return 0;
  }

  const char* data = buffer_file_readp (&ar->bf, filesize);
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
  if (f == NULL) {
    return 0;
  }
  /* Record the name. */
  f->name = malloc (namesize);
  if (f->name == NULL) {
    free (f);
    return 0;
  }
  memcpy (f->name, name, namesize);
  f->name_size = namesize;
  f->mode = from_hex (h->mode);
  /* Create a buffer and copy the file content. */
  f->bd = buffer_create (size_to_pages (filesize));
  if (f->bd == -1) {
    free (f->name);
    free (f);
    return 0;
  }
  memcpy (buffer_map (f->bd), data, filesize);
  if (buffer_unmap (f->bd) != 0) {
    free (f->name);
    free (f);
    return 0;
  }
  f->size = filesize;

  return f;
}

int
cpio_file_destroy (cpio_file_t* file)
{
  if (buffer_destroy (file->bd) != 0) {
    return -1;
  }
  free (file->name);
  free (file);
  return 0;
}
