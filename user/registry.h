#ifndef REGISTRY_H
#define REGISTRY_H

#include <stddef.h>
#include <lily/types.h>

/* Result codes. */
typedef enum {
  REGISTRY_SUCCESS,
  REGISTRY_NO_BUFFER,
  REGISTRY_BUFFER_TOO_BIG,
  REGISTRY_BAD_REQUEST,
  REGISTRY_UNKNOWN_METHOD,
  REGISTRY_NOT_UNIQUE,
} registry_error_t;

typedef enum {
  REGISTRY_STRING_EQUAL,
} registry_method_t;

typedef struct {
  registry_method_t method;	/* Comparison method. */
  ptrdiff_t description;	/* Relative offset to the description string. */
  size_t description_size;	/* Size of the description string. */
} registry_register_request_t;

typedef struct {
  registry_error_t error;
} registry_register_response_t;

typedef struct {
  registry_method_t method;	/* Comparison method. */
  ptrdiff_t specification;	/* Relative offset to the specification string. */
  size_t specification_size;	/* Size of the specification string. */
} registry_query_request_t;

typedef struct {
  aid_t aid;			/* Aid of matching automaton. */
  ptrdiff_t description;	/* Description of matching automaton. */
  size_t description_size;	/* Size of the description. */
  ptrdiff_t next;		/* Relative offset to the next entry in a linked list. */
} registry_query_result_t;

typedef struct {
  registry_error_t error;
  registry_method_t method;	/* Comparison method of the query. */
  ptrdiff_t result;		/* Relative offset to a registry_query_result_t. */
} registry_query_response_t;

#define REGISTER_REGISTER_REQUEST 1
#define REGISTER_REGISTER_RESPONSE 2

#define REGISTER_QUERY_REQUEST 3
#define REGISTER_QUERY_RESPONSE 4

#endif /* REGISTRY_H */
