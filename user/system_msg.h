#ifndef SYSTEM_MSG_H
#define SYSTEM_MSG_H

#include <lily/types.h>
#include <stdbool.h>

typedef struct {
  aid_t output_aid;
  ano_t output_ano;
  int output_parameter;
  aid_t input_aid;
  ano_t input_ano;
  int input_parameter;
  aid_t owner_aid;
} sa_binding_t;

void
sa_binding_init (sa_binding_t* b,
		 aid_t output_aid,
		 ano_t output_ano,
		 int output_parameter,
		 aid_t input_aid,
		 ano_t input_ano,
		 int input_parameter,
		 aid_t owner_aid);

bool
sa_binding_equal (const sa_binding_t* x,
		  const sa_binding_t* y);

typedef enum {
  SA_BINDING_INPUT,
  SA_BINDING_OUTPUT,
  SA_BINDING_OWNER,
} sa_binding_role_t;

typedef enum {
  SA_BINDING_SUCCESS,
  SA_BINDING_OAIDDNE,
  SA_BINDING_IAIDDNE,
  SA_BINDING_OANODNE,
  SA_BINDING_IANODNE,
  SA_BINDING_SAME,
  SA_BINDING_ALREADY,
  SA_BINDING_NOT_AUTHORIZED,
} sa_binding_outcome_t;

typedef struct {
  sa_binding_t binding;
} sa_bind_request_t;

void
sa_bind_request_init (sa_bind_request_t* req,
		      const sa_binding_t* b);

typedef struct {
  sa_binding_t binding;
  sa_binding_role_t role;
} sa_ba_request_t;

void
sa_ba_request_init (sa_ba_request_t* req,
		    const sa_binding_t* b,
		    sa_binding_role_t role);

typedef struct {
  sa_binding_t binding;
  sa_binding_role_t role;
  bool authorized;
} sa_ba_response_t;

void
sa_ba_response_init (sa_ba_response_t* res,
		     const sa_binding_t* b,
		     sa_binding_role_t role,
		     bool authorized);

typedef struct {
  sa_binding_t binding;
  sa_binding_role_t role;
  sa_binding_outcome_t outcome;
} sa_bind_result_t;

void
sa_bind_result_init (sa_bind_result_t* res,
		     const sa_binding_t* b,
		     sa_binding_role_t role,
		     sa_binding_outcome_t outcome);

#endif /* SYSTEM_MSG_H */
