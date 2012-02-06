#ifndef VGA_H
#define VGA_H

#include <stddef.h>

typedef enum {
  VGA_SET_START_ADDRESS,
  VGA_SET_CURSOR_LOCATION,
  VGA_ASSIGN,
} vga_op_type_t;

typedef struct {
  vga_op_type_t type;
  union {
    /* Move the start address to the specified location. */
    struct {
      size_t address;
    } set_start_address;
    /* Set the cursor. */
    struct {
      size_t location;
    } set_cursor_location;
    /* Assign size bytes of data starting at address.
       The data_offset field specifies the location of the data in the buffer. */
    struct {
      size_t address;
      size_t size;
      ptrdiff_t data; /* Offset of data in the buffer. */
    } assign;
  } arg;
  ptrdiff_t next; /* Offset of the next operation in the buffer. */
} vga_op_t;

#define VGA_FOCUS 1
#define VGA_OP 2

#endif /* VGA_H */
