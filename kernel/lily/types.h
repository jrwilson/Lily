#ifndef LILY_TYPES_H
#define LILY_TYPES_H

#include <stddef.h>

/* Automaton identifier. */
typedef int aid_t;
/* Binding identifier. */
typedef int bid_t;
/* Action number. */
typedef int ano_t;
/* Buffer descriptor. */
typedef int bd_t;

typedef struct {
  int type;
  int parameter_mode;
  ano_t number;
  size_t name_size;
  size_t description_size;
} action_descriptor_t;

#endif /* LILY_TYPES_H */
