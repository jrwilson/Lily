#ifndef VGA_H
#define VGA_H

#include <stddef.h>

typedef enum {
  VGA_SET_START_ADDRESS,
  VGA_ASSIGN,
} vga_op_type_t;

typedef struct {
  vga_op_type_t type;
  union {
    struct {
      size_t address;
    } set_start_address;
    struct {
      size_t address;
      size_t size;
      unsigned char data[0];
    } assign;
  } arg;
} vga_op_t;

#define VGA_FOCUS 1
#define VGA_OP 2

#endif /* VGA_H */
