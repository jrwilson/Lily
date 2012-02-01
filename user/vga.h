#ifndef VGA_H
#define VGA_H

#include <stddef.h>

typedef enum {
  VGA_SET_START,
  VGA_ASSIGN,
  VGA_COPY
} vga_op_type_t;

typedef struct {
  vga_op_type_t type;
  union {
    struct {
      size_t offset;
    } set_start;
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
} vga_op_t;

#define VGA_FOCUS 1
#define VGA_OP 2

#endif /* VGA_H */
