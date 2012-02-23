#ifndef REGISTRY_USER_H
#define REGISTRY_USER_H

#include <stddef.h>
#include <lily/types.h>
#include <buffer_file.h>

/* Result codes. */
typedef enum {
  REGISTRY_SUCCESS,
  REGISTRY_BAD_REQUEST,
  REGISTRY_UNKNOWN_METHOD,
  REGISTRY_NOT_UNIQUE,
} registry_error_t;

/* Methods for comparing descriptions and specifications. */
typedef enum {
  REGISTRY_STRING_EQUAL,
} registry_method_t;

bd_t
write_registry_register_request (registry_method_t method,
				 const void* description,
				 size_t size,
				 size_t* bd_size);

int
read_registry_register_request (bd_t bd,
				size_t bd_size,
				registry_method_t* method,
				const void** description,
				size_t* size);

bd_t
write_registry_register_response (registry_error_t error,
				  size_t* bd_size);

int
read_registry_register_response (bd_t bd,
				 size_t bd_size,
				 registry_error_t* error);

bd_t
write_registry_query_request (registry_method_t method,
			      const void* specification,
			      size_t size,
			      size_t* bd_size);

int
read_registry_query_request (bd_t bd,
			     size_t bd_size,
			     registry_method_t* method,
			     const void** specification,
			     size_t* size);

typedef struct {
  buffer_file_t bf;
  size_t count;
  int count_offset;
} registry_query_response_t;

bd_t
registry_query_response_initw (registry_query_response_t* r,
			       registry_error_t error,
			       registry_method_t method);

int
registry_query_response_append (registry_query_response_t* r,
				aid_t aid,
				const void* description,
				size_t size);

size_t
registry_query_response_bd_size (const registry_query_response_t* r);

int
registry_query_response_initr (registry_query_response_t* r,
			       bd_t bd,
			       size_t bd_size,
			       registry_error_t* error,
			       registry_method_t* method,
			       size_t* count);

int
registry_query_response_read (registry_query_response_t* r,
			      aid_t* aid,
			      const void** description,
			      size_t* size);

/* Action names. */
#define REGISTRY_REGISTER_REQUEST_NAME "register_request"
#define REGISTRY_REGISTER_RESPONSE_NAME "register_response"
#define REGISTRY_QUERY_REQUEST_NAME "query_request"
#define REGISTRY_QUERY_RESPONSE_NAME "query_response"

#endif /* REGISTRY_USER_H */
