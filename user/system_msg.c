#include "system_msg.h"

#include <automaton.h>
#include <dymem.h>
#include <string.h>

sa_create_request_t*
sa_create_request_create (bd_t text_bd,
			  bd_t bda,
			  bd_t bdb,
			  bool retain_privilege,
			  aid_t owner_aid)
{
  sa_create_request_t* req = malloc (sizeof (sa_create_request_t));
  memset (req, 0, sizeof (sa_create_request_t));
  req->text_bd = buffer_copy (text_bd);
  req->bda = buffer_copy (bda);
  req->bdb = buffer_copy (bdb);
  req->retain_privilege = retain_privilege;
  req->owner_aid = owner_aid;
  return req;
}

int
sa_create_request_write (buffer_file_t* bfa,
			 bd_t bd,
			 const sa_create_request_t* req)
{
  size_t text_bd_size = buffer_size (req->text_bd);
  size_t bda_size = buffer_size (req->bda);
  size_t bdb_size = buffer_size (req->bdb);

  if (buffer_file_write (bfa, &text_bd_size, sizeof (size_t)) != 0 ||
      buffer_file_write (bfa, &bda_size, sizeof (size_t)) != 0 ||
      buffer_file_write (bfa, &bdb_size, sizeof (size_t)) != 0 ||
      buffer_file_write (bfa, &req->retain_privilege, sizeof (bool)) != 0 ||
      buffer_file_write (bfa, &req->owner_aid, sizeof (aid_t)) != 0) {
    return -1;
  }

  buffer_resize (bd, 0);
  buffer_append (bd, req->text_bd);
  buffer_append (bd, req->bda);
  buffer_append (bd, req->bdb);

  return 0;
}

sa_create_request_t*
sa_create_request_read (buffer_file_t* bfa,
			bd_t bdb)
{
  size_t text_bd_size;
  size_t bda_size;
  size_t bdb_size;
  bool retain_privilege;
  aid_t owner_aid;
  if (buffer_file_read (bfa, &text_bd_size, sizeof (size_t)) != 0 ||
      buffer_file_read (bfa, &bda_size, sizeof (size_t)) != 0 ||
      buffer_file_read (bfa, &bdb_size, sizeof (size_t)) != 0 ||
      buffer_file_read (bfa, &retain_privilege, sizeof (bool)) != 0 ||
      buffer_file_read (bfa, &owner_aid, sizeof (aid_t)) != 0) {
    return 0;
  }

  const size_t size = buffer_size (bdb);
  if (size == -1) {
    return 0;
  }

  size_t size2 = 0;
  if (text_bd_size != -1) {
    size2 += text_bd_size;
  }
  if (bda_size != -1) {
    size2 += bda_size;
  }
  if (bdb_size != -1) {
    size2 += bdb_size;
  }

  if (size2 != size) {
    return 0;
  }

  /*
    [0, text_bd_size) -> text_bd
    [text_bd_size, text_bd_size + bda_size) -> bda
    [text_bd_size + bda_size, text_bd_size + bda_size + bdb_size) ->bdb
   */

  size_t offset = 0;

  sa_create_request_t* req = malloc (sizeof (sa_create_request_t));
  memset (req, 0, sizeof (sa_create_request_t));
  if (text_bd_size != -1) {
    req->text_bd = buffer_create (0);
    buffer_assign (req->text_bd, bdb, offset, offset + text_bd_size);
    offset += text_bd_size;
  }
  else {
    req->text_bd = -1;
  }
  if (bda_size != -1) {
    req->bda = buffer_create (0);
    buffer_assign (req->bda, bdb, offset, offset + bda_size);
    offset += bda_size;
  }
  else {
    req->bda = -1;
  }
  if (bdb_size != -1) {
    req->bdb = buffer_create (0);
    buffer_assign (req->bdb, bdb, offset, offset + bdb_size);
    offset += bdb_size;
  }
  else {
    req->bdb = -1;
  }

  req->retain_privilege = retain_privilege;
  req->owner_aid = owner_aid;
  return req;
}

void
sa_create_request_destroy (sa_create_request_t* req)
{
  buffer_destroy (req->text_bd);
  buffer_destroy (req->bda);
  buffer_destroy (req->bdb);
  free (req);
}

void
sa_ca_request_init (sa_ca_request_t* req)
{

}

void
sa_ca_response_init (sa_ca_response_t* res,
		     bool authorized)
{
  res->authorized = authorized;
}

void
sa_create_result_init (sa_create_result_t* res,
		       sa_create_outcome_t outcome)
{
  res->outcome = outcome;
}

void
sa_create_response_init (sa_create_response_t* res,
			 sa_create_outcome_t outcome,
			 aid_t aid)
{
  res->outcome = outcome;
  res->aid = aid;
}

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
		    sa_bind_role_t role)
{
  req->binding = *b;
  req->role = role;
}

void
sa_ba_response_init (sa_ba_response_t* res,
		     const sa_binding_t* b,
		     sa_bind_role_t role,
		     bool authorized)
{
  res->binding = *b;
  res->role = role;
  res->authorized = authorized;
}

void
sa_bind_result_init (sa_bind_result_t* res,
		     const sa_binding_t* b,
		     sa_bind_role_t role,
		     sa_bind_outcome_t outcome)
{
  res->binding = *b;
  res->role = role;
  res->outcome = outcome;
}

void
sa_bind_response_init (sa_bind_response_t* res,
		       const sa_binding_t* b,
		       sa_bind_outcome_t outcome)
{
  res->binding = *b;
  res->outcome = outcome;
}
