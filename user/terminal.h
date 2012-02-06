#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>


#define TERMINAL_FOCUS 1

#define TERMINAL_DISPLAY 2
typedef struct {
  size_t size;
  ptrdiff_t string; /* Relative offset. */
} terminal_display_arg_t;

#define TERMINAL_VGA_OP 3

#endif /* TERMINAL_H */
