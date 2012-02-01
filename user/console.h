#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>

typedef enum {
  CONSOLE_ASSIGN,
  CONSOLE_COPY
} console_op_type_t;

typedef struct {
  console_op_type_t type;
  union {
    struct {
      size_t offset;
      size_t size;
      unsigned char data[0];
    } assign;
    struct {
      size_t dst_offset;
      size_t src_offset;
      size_t size;
    } copy;
  } arg;
} console_op_t;

#define CONSOLE_FOCUS 1
#define CONSOLE_OP 2
#define CONSOLE_VGA_OP 3

#endif /* CONSOLE_H */
