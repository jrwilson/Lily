#ifndef SYSTEM_H
#define SYSTEM_H

#include <automaton.h>
#include <buffer_file.h>

/* The system automaton tries to bind to the following actions. */
#define SA_INIT_IN_NAME "init"

#include <automaton.h>

typedef struct automaton automaton_t;
typedef struct binding binding_t;
typedef struct globbed_binding globbed_binding_t;

typedef struct {
  buffer_file_t* output_bfa;
  bd_t bdb;
  ano_t bind_action;
  automaton_t* automaton_head;
  binding_t* binding_head;
  globbed_binding_t* globbed_binding_head;
  automaton_t* this;
  binding_t* bindq_head;
  binding_t** bindq_tail;
} system_t;

void
system_init (system_t* c,
	     buffer_file_t* output_bfa,
	     bd_t bdb,
	     ano_t bind_action);

void
system_bind_action (system_t* system);

void
system_schedule (system_t* system);

automaton_t*
system_get_this (system_t* c);

automaton_t*
system_add_managed_automaton (system_t* c,
			      bd_t text_bd,
			      bd_t bda,
			      bd_t bdb,
			      bool retain_privilege);

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
automaton_set_text (automaton_t* a,
		    bd_t text_bd);

bool
binding_bound (const binding_t* binding);

#endif /* SYSTEM_H */
