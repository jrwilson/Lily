#include "registry_msg.h"
#include <automaton.h>
#include <buffer_file.h>

bd_t
write_registry_register_request (registry_method_t method,
				 const void* description,
				 size_t size)
{
  size_t bd_sz = size_to_pages (sizeof (registry_method_t) + sizeof (size_t) + size);
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, bd, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, &method, sizeof (registry_method_t)) == -1 ||
      buffer_file_write (&file, &size, sizeof (size_t)) == -1 ||
      buffer_file_write (&file, description, size) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  return bd;
}

int
read_registry_register_request (bd_t bd,
				registry_method_t* method,
				const void** description,
				size_t* size)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&file, method, sizeof (registry_method_t)) == -1 ||
      buffer_file_read (&file, size, sizeof (size_t)) == -1) {
    return -1;
  }

  const void* d = buffer_file_readp (&file, *size);
  if (d == 0) {
    return -1;
  }

  *description = d;

  return 0;
}

bd_t
write_registry_register_response (registry_error_t error)
{
  size_t bd_sz = size_to_pages (sizeof (registry_error_t));
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, bd, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, &error, sizeof (registry_error_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  return bd;
}

int
read_registry_register_response (bd_t bd,
				 registry_error_t* error)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&file, error, sizeof (registry_error_t)) == -1) {
    return -1;
  }

  return 0;
}

bd_t
write_registry_query_request (registry_method_t method,
			      const void* specification,
			      size_t size)
{
  size_t bd_sz = size_to_pages (sizeof (registry_method_t) + sizeof (size_t) + size);
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  buffer_file_t file;
  if (buffer_file_open (&file, bd, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&file, &method, sizeof (registry_method_t)) == -1 ||
      buffer_file_write (&file, &size, sizeof (size_t)) == -1 ||
      buffer_file_write (&file, specification, size) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  return bd;
}

int
read_registry_query_request (bd_t bd,
			     registry_method_t* method,
			     const void** specification,
			     size_t* size)
{
  buffer_file_t file;
  if (buffer_file_open (&file, bd, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&file, method, sizeof (registry_method_t)) == -1 ||
      buffer_file_read (&file, size, sizeof (size_t)) == -1) {
    return -1;
  }

  const void* d = buffer_file_readp (&file, *size);
  if (d == 0) {
    return -1;
  }

  *specification = d;

  return 0;
}

bd_t
registry_query_response_initw (registry_query_response_t* r,
			       registry_error_t error,
			       registry_method_t method)
{
  size_t bd_sz = size_to_pages (sizeof (registry_error_t) + sizeof (registry_method_t) + sizeof (size_t));
  bd_t bd = buffer_create (bd_sz);
  if (bd == -1) {
    return -1;
  }

  if (buffer_file_open (&r->bf, bd, true) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  if (buffer_file_write (&r->bf, &error, sizeof (registry_error_t)) == -1 ||
      buffer_file_write (&r->bf, &method, sizeof (registry_method_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  /* Before we write the count, we save the position for future updates. */
  r->count_offset = buffer_file_seek (&r->bf, 0, BUFFER_FILE_CURRENT);

  r->count = 0;

  if (buffer_file_write (&r->bf, &r->count, sizeof (size_t)) == -1) {
    buffer_destroy (bd);
    return -1;
  }

  return bd;  
}

int
registry_query_response_append (registry_query_response_t* r,
				aid_t aid,
				const void* description,
				size_t size)
{
  if (buffer_file_write (&r->bf, &aid, sizeof (aid_t)) == -1 ||
      buffer_file_write (&r->bf, &size, sizeof (size_t)) == -1 ||
      buffer_file_write (&r->bf, description, size) == -1) {
    /* Fail. */
    return -1;
  }

  ++r->count;

  /* Save the current position. */
  int current = buffer_file_seek (&r->bf, 0, BUFFER_FILE_CURRENT);

  /* Go to the count. */
  buffer_file_seek (&r->bf, r->count_offset, BUFFER_FILE_SET);

  /* Write the count.  Should succeed because the backing store must exist and be mapped. */
  buffer_file_write (&r->bf, &r->count, sizeof (size_t));

  /* Go back to the current position. */
  buffer_file_seek (&r->bf, current, BUFFER_FILE_SET);

  return 0;
}

int
registry_query_response_initr (registry_query_response_t* r,
			       bd_t bd,
			       registry_error_t* error,
			       registry_method_t* method,
			       size_t* count)
{
  if (buffer_file_open (&r->bf, bd, false) == -1) {
    return -1;
  }

  if (buffer_file_read (&r->bf, error, sizeof (registry_error_t)) == -1 ||
      buffer_file_read (&r->bf, method, sizeof (registry_method_t)) == -1 ||
      buffer_file_read (&r->bf, count, sizeof (registry_method_t)) == -1) {
    return -1;
  }

  return 0;
}

int
registry_query_response_read (registry_query_response_t* r,
			      aid_t* aid,
			      const void** description,
			      size_t* size)
{
  if (buffer_file_read (&r->bf, aid, sizeof (aid_t)) == -1 ||
      buffer_file_read (&r->bf, size, sizeof (size_t)) == -1) {
    return -1;
  }

  const void* d = buffer_file_readp (&r->bf, *size);
  if (d == 0) {
    return -1;
  }

  *description = d;

  return 0;
}
