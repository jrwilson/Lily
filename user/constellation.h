#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include <automaton.h>

typedef struct automaton automaton_t;
typedef struct binding binding_t;
typedef struct globbed_binding globbed_binding_t;

typedef struct {
  automaton_t* automaton_head;
  binding_t* binding_head;
  globbed_binding_t* globbed_binding_head;
  automaton_t* this;
} constellation_t;

void
constellation_init (constellation_t* c);

automaton_t*
constellation_get_this (constellation_t* c);

automaton_t*
constellation_add_managed_automaton (constellation_t* c,
				     bd_t bd,
				     int retain_privilege);

automaton_t*
constellation_add_unmanaged_automaton (constellation_t* c,
				       aid_t aid);

binding_t*
constellation_add_binding (constellation_t* c,
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
constellation_add_globbed_binding (constellation_t* c,
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

#endif /* CONSTELLATION_H */
