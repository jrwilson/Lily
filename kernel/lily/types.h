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
  LILY_ERROR_PERM,
  LILY_ERROR_INVAL,
  LILY_ERROR_SRCNA,
  LILY_ERROR_SRCINUSE,
  LILY_ERROR_DSTINUSE,
  LILY_ERROR_NOMEM,
  LILY_ERROR_BDDNE,
  LILY_ERROR_BDSIZE,
  LILY_ERROR_ALIGN,
  LILY_ERROR_SIZE,
  LILY_ERROR_BADANO,
  LILY_ERROR_PORTINUSE,
  LILY_ERROR_AIDDNE,
  LILY_ERROR_MAPPED,
  LILY_ERROR_EMPTY,
  LILY_ERROR_OAIDDNE,
  LILY_ERROR_IAIDDNE,
  LILY_ERROR_OBADANO,
  LILY_ERROR_IBADANO,
  LILY_ERROR_ALREADY,
  LILY_ERROR_EXISTS,
  LILY_ERROR_BADTEXT,
  LILY_ERROR_NOTMAPPED,
  LILY_ERROR_NOTRESERVED,
  LILY_ERROR_NOTSUBSCRIBED,
  LILY_ERROR_BIDDNE,
  LILY_ERROR_NOTOWNER,
} lily_error_t;

#endif /* LILY_TYPES_H */
