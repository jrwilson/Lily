#ifndef CPIO_H
#define CPIO_H

#include <stddef.h>
#include <automaton.h>
#include <buffer_file.h>

typedef struct {
  char* name;
  size_t name_size;
  bd_t buffer;
  size_t buffer_size;
} cpio_file_t;

cpio_file_t*
parse_cpio (buffer_file_t* bf);

void
cpio_file_destroy (cpio_file_t* file);

#endif /* CPIO_H */
