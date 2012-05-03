#ifndef SYSTEM_H
#define SYSTEM_H

#include <automaton.h>
#include <buffer_file.h>

/* The system automaton tries to bind to the following actions. */
#define SA_BIND_REQUEST_OUT_NAME "sa_bind_request_out"
#define SA_BA_REQUEST_IN_NAME "sa_ba_request_in"
#define SA_BA_RESPONSE_OUT_NAME "sa_ba_response_out"
#define SA_BIND_RESULT_IN_NAME "sa_bind_result_in"

#include <automaton.h>

typedef struct automaton automaton_t;
typedef struct binding binding_t;
typedef struct globbed_binding globbed_binding_t;
typedef struct bind_request bind_request_t;

typedef struct {
  buffer_file_t* output_bfa;
  ano_t bind_request;
  automaton_t* automaton_head;
  binding_t* binding_head;
  globbed_binding_t* globbed_binding_head;
  automaton_t* this;
  bind_request_t* bind_request_head;
  bind_request_t** bind_request_tail;
} system_t;

void
system_init (system_t* c,
	     buffer_file_t* output_bfa,
	     ano_t bind_request);

void
system_bind_request (system_t* system);

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
		    int input_parameter,
		    automaton_t* owner_automaton);

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
