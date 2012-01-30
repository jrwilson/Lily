#ifndef VGA_H
#define VGA_H

typedef enum {
  ASSIGN_VALUE,
  ASSIGN_BUFFER,
  COPY
} op_type_t;

typedef struct {
  op_type_t type;
  union {
    struct {
      unsigned short offset;
      unsigned short count;
      unsigned short data[128];
    } assign_value;
  } arg;
} op_t;

#define VGA_FOCUS 1
#define VGA_OP_SENSE 2
#define VGA_OP 3

#endif /* VGA_H */
