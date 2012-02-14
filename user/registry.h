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

/* Methods for comparing descriptions and specifications. */
typedef enum {
  REGISTRY_STRING_EQUAL,
} registry_method_t;

typedef struct {
  registry_method_t method;	/* Comparison method. */
  size_t description_size;	/* Size of the description string. */
  				/* Description string follows. */
} registry_register_request_t;

typedef struct {
  registry_error_t error;
} registry_register_response_t;

typedef struct {
  registry_method_t method;	/* Comparison method. */
  size_t specification_size;	/* Size of the specification string. */
  				/* Specification string follows. */
} registry_query_request_t;

typedef struct {
  registry_error_t error;
  registry_method_t method;	/* Comparison method of the query. */
  size_t count;			/* The number of results to follow. */
} registry_query_response_t;

typedef struct {
  aid_t aid;			/* Aid of matching automaton. */
  size_t description_size;	/* Size of the description. */
				/* Description string follows. */
} registry_query_result_t;

#define REGISTER_REGISTER_REQUEST 1
#define REGISTER_REGISTER_RESPONSE 2

#define REGISTER_QUERY_REQUEST 3
#define REGISTER_QUERY_RESPONSE 4

#endif /* REGISTRY_H */
