#include "bind_auth.h"

#include <automaton.h>
#include <dymem.h>
#include <string.h>
#include "system_msg.h"

struct bind_auth_response {
  sa_ba_response_t response;
  bind_auth_response_t* next;
};

void
bind_auth_init (bind_auth_t* ba,
		buffer_file_t* output_bfa,
		ano_t ba_response)
{
  ba->output_bfa = output_bfa;
  ba->ba_response = ba_response;
  ba->response_head = 0;
  ba->response_tail = &ba->response_head;
}

static void
push_ba_response (bind_auth_t* ba,
		  void* id,
		  sa_bind_role_t role,
		  bool authorized)
{
  bind_auth_response_t* res = malloc (sizeof (bind_auth_response_t));
  memset (res, 0, sizeof (bind_auth_response_t));
  sa_ba_response_init (&res->response, id, role, authorized);
  *ba->response_tail = res;
  ba->response_tail = &res->next;
}

static void
pop_ba_response (bind_auth_t* ba)
{
  bind_auth_response_t* res = ba->response_head;
  ba->response_head = res->next;
  if (ba->response_head == 0) {
    ba->response_tail = &ba->response_head;
  }
  free (res);
}

void
bind_auth_request (bind_auth_t* ba,
		   bd_t bda,
		   bd_t bdb)
{
  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }

  const sa_ba_request_t* req = buffer_file_readp (&bf, sizeof (sa_ba_request_t));
  if (req == 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }

  /* TODO:  This is where we should apply whatever policy the user want.
     For now, we will authorize everything. */

  push_ba_response (ba, req->id, req->role, true);

  finish_input (bda, bdb);
}

static bool
response_precondition (bind_auth_t* ba)
{
  return ba->response_head != 0;
}

void
bind_auth_response (bind_auth_t* ba)
{
  if (response_precondition (ba)) {
    // Write the response.
    buffer_file_shred (ba->output_bfa);
    buffer_file_write (ba->output_bfa, &ba->response_head->response, sizeof (sa_ba_response_t));

    // Pop the request.
    pop_ba_response (ba);

    finish_output (true, ba->output_bfa->bd, -1);
  }
  finish_output (false, -1, -1);
}

void
bind_auth_schedule (bind_auth_t* ba)
{
  if (response_precondition (ba)) {
    schedule (ba->ba_response, 0);
  }
}
