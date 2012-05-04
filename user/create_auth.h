#ifndef CREATE_AUTH_H
#define CREATE_AUTH_H

#include <buffer_file.h>

typedef struct create_auth_response create_auth_response_t;

typedef struct {
  buffer_file_t* output_bfa;
  ano_t ca_response;
  create_auth_response_t* response_head;
  create_auth_response_t** response_tail;
} create_auth_t;

void
create_auth_init (create_auth_t* ba,
		  buffer_file_t* output_bfa,
		  ano_t ba_response);

void
create_auth_request (create_auth_t* ba,
		     bd_t bda,
		     bd_t bdb);

void
create_auth_response (create_auth_t* ba);

void
create_auth_schedule (create_auth_t* ba);

#endif /* CREATE_AUTH_H */
