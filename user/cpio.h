#ifndef CPIO_H
#define CPIO_H

#include <stddef.h>
#include <automaton.h>
#include <buffer_file.h>


#define CPIO_PERM_MASK  0x000001FF
#define CPIO_STICKY_BIT 0x00000200
#define CPIO_SGID_BIT   0x00000400
#define CPIO_SUID_BIT   0x00000800
#define CPIO_TYPE_MASK  0x0000F000

#define CPIO_SOCKET     0x0000C000
#define CPIO_SYMLINK    0x0000A000
#define CPIO_REGULAR    0x00008000
#define CPIO_BLOCK      0x00006000
#define CPIO_DIRECTORY  0x00004000
#define CPIO_CHARACTER  0x00002000
#define CPIO_FIFO       0x00001000

typedef struct {
  char* name;
  size_t name_size;
  unsigned int mode;
  bd_t bd;
  size_t bd_size;
  size_t size;
} cpio_file_t;

typedef struct {
  buffer_file_t bf;
} cpio_archive_t;

int
cpio_archive_init (cpio_archive_t* ar,
		   bd_t bd);

cpio_file_t*
cpio_archive_next_file (cpio_archive_t* ar);

void
cpio_file_destroy (cpio_file_t* file);

#endif /* CPIO_H */
