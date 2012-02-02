#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

typedef enum {
  CONSOLE_APPEND,
  CONSOLE_REPLACE_LAST,
} console_op_type_t;

typedef struct {
  console_op_type_t type;
  union {
    struct {
      size_t line_count;
      unsigned char data[0];
    } append;
    struct {
      unsigned char data[0];
    } replace_last;
  } arg;
} console_op_t;

#define CONSOLE_FOCUS 1
#define CONSOLE_OP 2
#define CONSOLE_VGA_OP 3

#endif /* CONSOLE_H */
