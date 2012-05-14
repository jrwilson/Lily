#ifndef SYSTEM_H
#define SYSTEM_H

#include <automaton.h>

/* The system automaton tries to bind to the following actions. */
#define SA_SYSTEM_ACTION_NAME "system_action"
#define SA_INIT_OUT_NAME "init_out"
#define SA_INIT_IN_NAME "init_in"
#define SA_STATUS_REQUEST_OUT_NAME "status_request_out"
#define SA_STATUS_REQUEST_IN_NAME "status_request_in"
#define SA_STATUS_RESPONSE_OUT_NAME "status_response_out"
#define SA_STATUS_RESPONSE_IN_NAME "status_response_in"
#define SA_BINDING_UPDATE_OUT_NAME "binding_update_out"
#define SA_BINDING_UPDATE_IN_NAME "binding_update_in"

typedef struct action action_t;
typedef struct automaton automaton_t;
typedef struct binding binding_t;
typedef struct create_item create_item_t;
typedef struct bind_item bind_item_t;
typedef struct init_item init_item_t;
/* typedef struct globbed_binding globbed_binding_t; */

typedef void (*create_callback_t) (void* arg, automaton_t* a);

typedef struct {
  automaton_t* automaton_head;
  binding_t* binding_head;
  create_item_t* createq_head;
  create_item_t** createq_tail;
  bind_item_t* bindq_head;
  bind_item_t** bindq_tail;
  init_item_t* initq_head;
  init_item_t** initq_tail;
  automaton_t* this;
  ano_t system_action;
  ano_t init_out;
  /* ano_t status_request_out; */
  /* ano_t status_response_out; */
  /* ano_t binding_update_out; */
  bd_t init_bda;
  bd_t init_bdb;
  /* globbed_binding_t* globbed_binding_head; */
} system_t;

int
system_init (system_t* system);

/* void */
/* system_status_request_out (system_t* system, */
/* 			   aid_t aid); */

/* void */
/* system_status_request_in (system_t* system, */
/* 			  aid_t aid, */
/* 			  bd_t bda, */
/* 			  bd_t bdb); */

/* void */
/* system_status_response_out (system_t* system, */
/* 			    aid_t aid); */

/* void */
/* system_status_response_in (system_t* system, */
/* 			   aid_t aid, */
/* 			   bd_t bda, */
/* 			   bd_t bdb); */

/* void */
/* system_binding_update_out (system_t* system, */
/* 			   aid_t aid); */

/* void */
/* system_binding_update_in (system_t* system, */
/* 			  aid_t aid, */
/* 			  bd_t bda, */
/* 			  bd_t bdb); */

/* /\* binding_t* *\/ */
/* /\* system_add_binding (system_t* c, *\/ */
/* /\* 		    automaton_t* output_automaton, *\/ */
/* /\* 		    const char* output_action_begin, *\/ */
/* /\* 		    const char* output_end_end, *\/ */
/* /\* 		    ano_t output_action, *\/ */
/* /\* 		    int output_parameter, *\/ */
/* /\* 		    automaton_t* input_automaton, *\/ */
/* /\* 		    const char* input_action_begin, *\/ */
/* /\* 		    const char* input_end_end, *\/ */
/* /\* 		    ano_t input_action, *\/ */
/* /\* 		    int input_parameter); *\/ */

/* /\* globbed_binding_t* *\/ */
/* /\* system_add_globbed_binding (system_t* c, *\/ */
/* /\* 			    automaton_t* output_automaton, *\/ */
/* /\* 			    const char* output_action_begin, *\/ */
/* /\* 			    const char* output_end_end, *\/ */
/* /\* 			    int output_parameter, *\/ */
/* /\* 			    automaton_t* input_automaton, *\/ */
/* /\* 			    const char* input_action_begin, *\/ */
/* /\* 			    const char* input_end_end, *\/ */
/* /\* 			    int input_parameter); *\/ */

/* /\* bool *\/ */
/* /\* binding_bound (const binding_t* binding); *\/ */

automaton_t*
system_create (system_t* system,
	       bd_t text_bd,
	       bool retain_privilege,
	       bd_t bda,
	       bd_t bdb,
	       create_callback_t callback,
	       void* arg);

aid_t
automaton_aid (const automaton_t* a);

lily_error_t
automaton_error (const automaton_t* a);

void
system_system_action (system_t* system);

void
system_init_out (system_t* system,
		 aid_t aid);

void
system_schedule (const system_t* system);


#endif /* SYSTEM_H */
