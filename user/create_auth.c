#include "create_auth.h"

#include <automaton.h>
#include <dymem.h>
#include <string.h>
#include "system_msg.h"

struct create_auth_response {
  sa_ca_response_t response;
  create_auth_response_t* next;
};

void
create_auth_init (create_auth_t* ca,
		  buffer_file_t* output_bfa,
		  ano_t ca_response)
{
  ca->output_bfa = output_bfa;
  ca->ca_response = ca_response;
  ca->response_head = 0;
  ca->response_tail = &ca->response_head;
}

static void
push_ca_response (create_auth_t* ca,
		  bool authorized)
{
  create_auth_response_t* res = malloc (sizeof (create_auth_response_t));
  memset (res, 0, sizeof (create_auth_response_t));
  sa_ca_response_init (&res->response, authorized);
  *ca->response_tail = res;
  ca->response_tail = &res->next;
}

static void
pop_ca_response (create_auth_t* ca)
{
  create_auth_response_t* res = ca->response_head;
  ca->response_head = res->next;
  if (ca->response_head == 0) {
    ca->response_tail = &ca->response_head;
  }
  free (res);
}

void
create_auth_request (create_auth_t* ca,
		     bd_t bda,
		     bd_t bdb)
{
  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }
  
  const sa_ca_request_t* req = buffer_file_readp (&bf, sizeof (sa_ca_request_t));
  if (req == 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }
  
  /* TODO:  This is where we should apply whatever policy the user want.
     For now, we will authorize everything. */
  
  push_ca_response (ca, true);

  finish_input (bda, bdb);
}

static bool
response_precondition (create_auth_t* ca)
{
  return ca->response_head != 0;
}

void
create_auth_response (create_auth_t* ca)
{
  if (response_precondition (ca)) {
    // Write the response.
    buffer_file_shred (ca->output_bfa);
    buffer_file_write (ca->output_bfa, &ca->response_head->response, sizeof (sa_ca_response_t));
    
    // Pop the request.
    pop_ca_response (ca);
    
    finish_output (true, ca->output_bfa->bd, -1);
  }
  finish_output (false, -1, -1);
}

void
create_auth_schedule (create_auth_t* ca)
{
  if (response_precondition (ca)) {
    schedule (ca->ca_response, 0);
  }
}
