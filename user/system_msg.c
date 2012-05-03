#include "system_msg.h"

void
sa_binding_init (sa_binding_t* b,
		 aid_t output_aid,
		 ano_t output_ano,
		 int output_parameter,
		 aid_t input_aid,
		 ano_t input_ano,
		 int input_parameter,
		 aid_t owner_aid)
{
  b->output_aid = output_aid;
  b->output_ano = output_ano;
  b->output_parameter = output_parameter;
  b->input_aid = input_aid;
  b->input_ano = input_ano;
  b->input_parameter = input_parameter;
  b->owner_aid = owner_aid;
}

bool
sa_binding_equal (const sa_binding_t* x,
		  const sa_binding_t* y)
{
  return x->output_aid == y->output_aid &&
    x->output_ano == y->output_ano &&
    x->output_parameter == y->output_parameter &&
    x->input_aid == y->input_aid &&
    x->input_ano == y->input_ano &&
    x->input_parameter == y->input_parameter &&
    x->owner_aid == y->owner_aid;
}

void
sa_bind_request_init (sa_bind_request_t* req,
		      const sa_binding_t* b)
{
  req->binding = *b;
}

void
sa_ba_request_init (sa_ba_request_t* req,
		    const sa_binding_t* b,
		    sa_binding_role_t role)
{
  req->binding = *b;
  req->role = role;
}

void
sa_ba_response_init (sa_ba_response_t* res,
		     const sa_binding_t* b,
		     sa_binding_role_t role,
		     bool authorized)
{
  res->binding = *b;
  res->role = role;
  res->authorized = authorized;
}

void
sa_bind_result_init (sa_bind_result_t* res,
		     const sa_binding_t* b,
		     sa_binding_role_t role,
		     sa_binding_outcome_t outcome)
{
  res->binding = *b;
  res->role = role;
  res->outcome = outcome;
}
