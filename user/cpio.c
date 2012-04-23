#include "cpio.h"
#include <string.h>

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

int
cpio_archive_read (cpio_archive_t* ar,
		   cpio_file_t* f)
{
  /* Align to a 4-byte boundary. */
  size_t pos = buffer_file_position (&ar->bf);
  pos = align_up (pos, 4);
  if (buffer_file_seek (&ar->bf, pos) != 0) {
    return -1;
  }

  const cpio_header_t* h = buffer_file_readp (&ar->bf, sizeof (cpio_header_t));
  if (h == 0) {
    /* Underflow. */
    return -1;
  }

  if (memcmp (h->magic, "070701", 6) == 0) {

  }
  else if (memcmp (h->magic, "070702", 6) == 0) {

  }
  else {
    /* Bad magic number. */
    return -1;
  }
  f->mode = from_hex (h->mode);
  f->file_size = from_hex (h->filesize);
  f->name_size = from_hex (h->namesize);

  f->name = buffer_file_readp (&ar->bf, f->name_size);
  if (f->name == 0) {
    return -1;
  }
  if (f->name[f->name_size - 1] != 0) {
    /* Name is not null terminated. */
    return -1;
  }

  /* Check for the trailer. */
  if (strcmp (f->name, "TRAILER!!!") == 0) {
    /* Done. */
    return -1;
  }

  /* Align to a 4-byte boundary. */
  pos = buffer_file_position (&ar->bf);
  pos = align_up (pos, 4);
  if (buffer_file_seek (&ar->bf, pos) != 0) {
    return -1;
  }

  /* Skip over the data. */
  f->position = buffer_file_position (&ar->bf);
  const char* data = buffer_file_readp (&ar->bf, f->file_size);
  if (data == 0) {
    return -1;
  }

  return 0;
}

bd_t
cpio_file_read (cpio_archive_t* ar,
		const cpio_file_t* f)
{
  /* Get the current position. */
  size_t position = buffer_file_position (&ar->bf);

  /* Seek to the file. */
  if (buffer_file_seek (&ar->bf, f->position) != 0) {
    return -1;
  }

  /* Read the file. */
  bd_t bd = buffer_create (size_to_pages (f->file_size));
  if (bd == -1) {
    return -1;
  }
  void* ptr = buffer_map (bd);
  if (ptr == 0) {
    buffer_destroy (bd);
    return -1;
  }
  if (buffer_file_read (&ar->bf, ptr, f->file_size) != 0) {
    buffer_destroy (bd);
    return -1;
  }
  if (buffer_unmap (bd) != 0) {
    buffer_destroy (bd);
    return -1;
  }

  /* Seek to the original position. */
  if (buffer_file_seek (&ar->bf, position) != 0) {
    buffer_destroy (bd);
    return -1;
  }

  return bd;
}
