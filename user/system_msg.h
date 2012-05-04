#ifndef SYSTEM_MSG_H
#define SYSTEM_MSG_H

#include <lily/types.h>
#include <stdbool.h>
#include <buffer_file.h>

typedef struct {
  bd_t text_bd;
  bd_t bda;
  bd_t bdb;
  bool retain_privilege;
  aid_t owner_aid;
} sa_create_request_t;

sa_create_request_t*
sa_create_request_create (bd_t text_bd,
			  bd_t bda,
			  bd_t bdb,
			  bool retain_privilege,
			  aid_t owner_aid);

int
sa_create_request_write (buffer_file_t* bfa,
			 const sa_create_request_t* req);

void
sa_create_request_destroy (sa_create_request_t* req);

typedef enum {
  SA_CREATE_SUCCESS,
  SA_CREATE_NOT_AUTHORIZED,
  SA_CREATE_INVAL,
  SA_CREATE_BDDNE,
} sa_create_outcome_t;

typedef struct {
} sa_ca_request_t;

void
sa_ca_request_init (sa_ca_request_t* req);

typedef struct {
  bool authorized;
} sa_ca_response_t;

void
sa_ca_response_init (sa_ca_response_t* res,
		     bool authorized);

typedef struct {
  sa_create_outcome_t outcome;
} sa_create_result_t;

void
sa_create_result_init (sa_create_result_t* res,
		       sa_create_outcome_t outcome);

typedef struct {
  sa_create_outcome_t outcome;
} sa_create_response_t;

void
sa_create_response_init (sa_create_response_t* res,
			 sa_create_outcome_t outcome);

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
  SA_BIND_INPUT,
  SA_BIND_OUTPUT,
  SA_BIND_OWNER,
} sa_bind_role_t;

typedef enum {
  SA_BIND_SUCCESS,
  SA_BIND_OAIDDNE,
  SA_BIND_IAIDDNE,
  SA_BIND_OANODNE,
  SA_BIND_IANODNE,
  SA_BIND_SAME,
  SA_BIND_ALREADY,
  SA_BIND_NOT_AUTHORIZED,
} sa_bind_outcome_t;

typedef struct {
  sa_binding_t binding;
} sa_bind_request_t;

void
sa_bind_request_init (sa_bind_request_t* req,
		      const sa_binding_t* b);

typedef struct {
  sa_binding_t binding;
  sa_bind_role_t role;
} sa_ba_request_t;

void
sa_ba_request_init (sa_ba_request_t* req,
		    const sa_binding_t* b,
		    sa_bind_role_t role);

typedef struct {
  sa_binding_t binding;
  sa_bind_role_t role;
  bool authorized;
} sa_ba_response_t;

void
sa_ba_response_init (sa_ba_response_t* res,
		     const sa_binding_t* b,
		     sa_bind_role_t role,
		     bool authorized);

typedef struct {
  sa_binding_t binding;
  sa_bind_role_t role;
  sa_bind_outcome_t outcome;
} sa_bind_result_t;

void
sa_bind_result_init (sa_bind_result_t* res,
		     const sa_binding_t* b,
		     sa_bind_role_t role,
		     sa_bind_outcome_t outcome);

typedef struct {
  sa_binding_t binding;
  sa_bind_outcome_t outcome;
} sa_bind_response_t;

void
sa_bind_response_init (sa_bind_response_t* res,
		       const sa_binding_t* b,
		       sa_bind_outcome_t outcome);

#endif /* SYSTEM_MSG_H */
