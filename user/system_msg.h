#ifndef SYSTEM_MSG_H
#define SYSTEM_MSG_H

#include <automaton.h>

typedef struct {
  bd_t text;
  bd_t bda;
  bd_t bdb;
  int retain_privilege;
} create_t;

void
create_initr (create_t* c,
	      bd_t bda,
	      bd_t bdb);

void
create_fini (create_t* c);

typedef struct {
  aid_t output_aid;
  ano_t output_ano;
  int output_parameter;
  aid_t input_aid;
  ano_t input_ano;
  int input_parameter;
} bind_t;

void
bind_initr (bind_t* b,
	    bd_t bd);

void
bind_fini (bind_t* b);

typedef struct {
  bid_t bid;
} unbind_t;

void
unbind_initr (unbind_t* u,
	      bd_t bd);

void
unbind_fini (unbind_t* u);

typedef struct {
  aid_t aid;
} destroy_t;

void
destroy_initr (destroy_t* d,
	       bd_t bd);

void
destroy_fini (destroy_t* d);

#endif /* SYSTEM_MSG_H */
