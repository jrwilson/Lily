#ifndef BIND_AUTH_H
#define BIND_AUTH_H

#include <buffer_file.h>

typedef struct bind_auth_response bind_auth_response_t;

typedef struct {
  buffer_file_t* output_bfa;
  ano_t ba_response;
  bind_auth_response_t* response_head;
  bind_auth_response_t** response_tail;
} bind_auth_t;

void
bind_auth_init (bind_auth_t* ba,
		buffer_file_t* output_bfa,
		ano_t ba_response);

void
bind_auth_request (bind_auth_t* ba,
		   bd_t bda,
		   bd_t bdb);

void
bind_auth_response (bind_auth_t* ba);
		   
void
bind_auth_schedule (bind_auth_t* ba);

#endif /* BIND_AUTH_H */
