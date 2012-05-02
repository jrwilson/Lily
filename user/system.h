#ifndef SYSTEM_H
#define SYSTEM_H

#include <automaton.h>
#include <buffer_file.h>

#define SYSTEM_REQUEST_OUT_NAME "system_request_out"
#define SYSTEM_RESPONSE_IN_NAME "system_response_in"

#include <automaton.h>

typedef struct automaton automaton_t;
typedef struct binding binding_t;
typedef struct globbed_binding globbed_binding_t;
typedef struct system_op system_op_t;

typedef struct {
  ano_t request;
  ano_t response;
  automaton_t* automaton_head;
  binding_t* binding_head;
  globbed_binding_t* globbed_binding_head;
  automaton_t* this;
  system_op_t* send_head;
  system_op_t** send_tail;
  system_op_t* recv_head;
  system_op_t** recv_tail;
  bd_t bda;
  buffer_file_t bfa;
} system_t;

void
system_init (system_t* c,
	     ano_t request,
	     ano_t response);

void
system_request (system_t* system);

void
system_response (system_t* system,
		 bd_t bda,
		 bd_t bdb);

void
system_schedule (system_t* system);

automaton_t*
system_get_this (system_t* c);

automaton_t*
system_add_managed_automaton (system_t* c,
			      bd_t bd,
			      int retain_privilege);

automaton_t*
system_add_unmanaged_automaton (system_t* c,
				       aid_t aid);

binding_t*
system_add_binding (system_t* c,
			   automaton_t* output_automaton,
			   const char* output_action_begin,
			   const char* output_end_end,
			   ano_t output_action,
			   int output_parameter,
			   automaton_t* input_automaton,
			   const char* input_action_begin,
			   const char* input_end_end,
			   ano_t input_action,
			   int input_parameter);

globbed_binding_t*
system_add_globbed_binding (system_t* c,
				   automaton_t* output_automaton,
				   const char* output_action_begin,
				   const char* output_end_end,
				   int output_parameter,
				   automaton_t* input_automaton,
				   const char* input_action_begin,
				   const char* input_end_end,
				   int input_parameter);

void
automaton_create (automaton_t* a,
		  bd_t bd);

#endif /* SYSTEM_H */
