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

typedef struct {
  unsigned int seconds;
  unsigned int nanoseconds;
} mono_time_t;

/* Error codes. */
typedef enum {
  LILY_ERROR_SUCCESS,
  LILY_ERROR_INVAL,
  LILY_ERROR_ALREADY,
  LILY_ERROR_NOT,
  LILY_ERROR_PERMISSION,
  LILY_ERROR_AIDDNE,
  LILY_ERROR_BIDDNE,
  LILY_ERROR_ANODNE,
  LILY_ERROR_BDDNE,
  LILY_ERROR_NOMEM,
  LILY_ERROR_OAIDDNE,
  LILY_ERROR_IAIDDNE,
  LILY_ERROR_SAME,
  LILY_ERROR_OANODNE,
  LILY_ERROR_IANODNE,
} lily_error_t;

typedef struct {
  aid_t aid;
  mono_time_t time;
  size_t message_size;
  char message[];
} log_event_t;

#endif /* LILY_TYPES_H */
