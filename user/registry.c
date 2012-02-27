#include "registry_msg.h"
#include <automaton.h>
#include <io.h>
#include <string.h>
#include <dymem.h>
#include <fifo_scheduler.h>
#include <buffer_queue.h>

/*
  The Registry Automaton
  ======================
  The registry is a database of automata.
  The goal of the registry is to allow independent automata to find each other.
  For example, an automaton that manipulates files needs to find an automaton that serves those files, e.g., a virtual file system automaton.

  The two main activities involving the registry are registration and query.
  For registration, an automaton submits a description and algorithm to use when matching.
  The registry associates the description and algorithm with the automaton identifier (aid) of the registering automaton.
  A description for the same automaton and algorithm overwrites an existing description.
  For query, an automaton submits a specification and matching algorithm.
  Using the algorithm, the registry compares the specification to descriptions registered with the same algorithm and returns a list of matching descriptions and aids.

  The motivation behind specifying the matching algorithm is to provide flexibility.
  The simplest algorithm compares the descriptions for string equality.
  A more complex algorithm might search for automata that contain certain actions or behaviors.
  The key point is that new algorithms can be added as needed.

  Identical description/algorithm pairs are not allowed to support the specical case of singleton automata with string equality matching.
  Assume that string equality is being used for matching and that the system is supposed to have one "abc" automaton.
  Suppose that due to misconfiguration, two automata have the description "abc."
  Another automaton wishing to use the "abc" automaton must make a decision; the outcome of which is probably not what is expected or desired.
  The descriptions for more flexible matching algorithms can be made unique by including an aid or some other unique piece of information.

  Comparison Methods
  ------------------
  STRING_EQUAL - byte-wise comparison of the specification and description.  (Sizes must match.)

  Implementation Details
  ----------------------
  The descriptions are stored in a linked-list.
  If performance becomes an issue, then more advanced data structures should be considered.

  Matching is performed when the specification is received.
  One could increase concurrency by introducing an internal action that performs the matching.
  This should only be necessary if the matching process becomes appreciably complex.

  Bugs and Limitations
  --------------------
  * We don't account for the possibility of huge descriptions/specifications.
  * Error checking is lacking, e.g., malloc.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define REGISTRY_REGISTER_REQUEST_NO 1
#define REGISTRY_REGISTER_RESPONSE_NO 2
#define REGISTRY_QUERY_REQUEST_NO 3
#define REGISTRY_QUERY_RESPONSE_NO 4
#define REGISTRY_DESTROYED_NO 5
#define DESTROY_BUFFERS_NO 6

typedef struct description_struct description_t;
struct description_struct {
  aid_t aid;
  registry_method_t method;
  void* description;
  size_t description_size;
  description_t* next;
};

static description_t* description_head = 0;

static description_t*
create_description (aid_t aid,
		    registry_method_t method,
		    const void* description,
		    size_t description_size)
{
  description_t* d = malloc (sizeof (description_t));
  memset (d, 0, sizeof (description_t));
  d->aid = aid;
  d->method = method;
  d->description = malloc (description_size);
  memcpy (d->description, description, description_size);
  d->description_size = description_size;
  return d;
}

static void
destroy_description (description_t* d)
{
  free (d->description);
  free (d);
}

static void
description_replace_description (description_t* d,
				 const void* description,
				 size_t description_size)
{
  free (d->description);
  d->description = malloc (description_size);
  memcpy (d->description, description, description_size);
  d->description_size = description_size;
}

/* Initialization flag. */
static bool initialized = false;

/* Queue for register responses. */
static buffer_queue_t rr_queue;

/* Queue for query responses. */
static buffer_queue_t qr_queue;

/* Queue of buffers to destroy. */
static buffer_queue_t destroy_queue;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
    buffer_queue_init (&rr_queue);
    buffer_queue_init (&qr_queue);
    buffer_queue_init (&destroy_queue);
  }
}

static void
ssyslog (const char* msg)
{
  syslog (msg, strlen (msg));
}

static bool
match (registry_method_t method,
       const void* description,
       size_t description_size,
       const void* specification,
       size_t specification_size)
{
  switch (method) {
  case REGISTRY_STRING_EQUAL:
    {
      return description_size == specification_size && memcmp (description, specification, description_size) == 0;
    }
    break;
  }

  return false;
}

static void
form_register_response (aid_t aid,
			registry_error_t error)
{
  /* Create a response. */
  bd_t bd = write_registry_register_response (error);
  buffer_queue_push (&rr_queue, aid, bd, -1);
}

static void
form_query_response (aid_t aid,
		     registry_error_t error,
		     registry_method_t method)
{
  /* Create a response. */
  registry_query_response_t qr;
  bd_t bd = registry_query_response_initw (&qr, error, method);

  buffer_queue_push (&qr_queue, aid, bd, -1);
}

static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb);

/* register_request
   ----------------
   Receive a registration request.

   Post: The end of the registration response queue will contain a response to the query.
         If the registration is successful, we will be subscribed to the automaton.
 */
BEGIN_INPUT (AUTO_PARAMETER, REGISTRY_REGISTER_REQUEST_NO, REGISTRY_REGISTER_REQUEST_NAME, "", register_request, aid_t aid, bd_t bda, bd_t bdb)
{
  ssyslog ("registry: register_request\n");
  initialize ();

  registry_method_t method;
  const void* description;
  size_t size;

  if (read_registry_register_request (bda, &method, &description, &size) == -1) {
    form_register_response (aid, REGISTRY_BAD_REQUEST);
    end_action (false, bda, bdb);
  }

  /* Check that no description is equal to the given description. */
  description_t* d;
  for (d = description_head; d != 0; d = d->next) {
    if (d->description_size == size &&
	memcmp (d->description, description, size) == 0) {
      /* Found a description that is equal. */
      break;
    }
  }
  if (d != 0) {
    form_register_response (aid, REGISTRY_NOT_UNIQUE);
    end_action (false, bda, bdb);
  }

  /* Inspect the method. */
  switch (method) {
  case REGISTRY_STRING_EQUAL:
    /* Okay. */
    break;
  default:
    form_register_response (aid, REGISTRY_UNKNOWN_METHOD);
    end_action (false, bda, bdb);
    break;
  }

  /* Insert a new description or replace an existing description. */
  for (d = description_head; d != 0; d = d->next) {
    if (d->aid == aid &&
	d->method == method) {
      /* Found a description for this automaton and method. */
      break;
    }
  }
  if (d == 0) {
    /* Create a new description. */
    d = create_description (aid, method, description, size);
    d->next = description_head;
    description_head = d;
  }
  else {
    /* Replace the existing description. */
    description_replace_description (d, description, size);
    free (d->description);
    d->description = malloc (size);
    memcpy (d->description, description, size);
    d->description_size = size;
  }

  /* This must succeed as they are bound to this input. */
  subscribe_destroyed (aid, REGISTRY_DESTROYED_NO);
  form_register_response (aid, REGISTRY_SUCCESS);

  end_action (false, bda, bdb);
}

/* register_response
   -----------------
   Relay the result of registration to a user.

   Pre:  the register response queue contains a response for the given automaton
   Post: the first response in the query response queue is removed and returned
 */
BEGIN_OUTPUT (AUTO_PARAMETER, REGISTRY_REGISTER_RESPONSE_NO, REGISTRY_REGISTER_RESPONSE_NAME, "", register_response, aid_t aid)
{
  initialize ();
  scheduler_remove (REGISTRY_REGISTER_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&rr_queue, aid);

  if (item != 0) {
    ssyslog ("registry: register_response\n");

    /* Found a response.  Execute. */
    bd_t bda = buffer_queue_item_bda (item);
    bd_t bdb = buffer_queue_item_bdb (item);
    buffer_queue_erase (&rr_queue, item);
    end_action (true, bda, bdb);
  }
  else {
    /* Did not find a response. */
    end_action (false, -1, -1);
  }
}

/* query_request
   -------------
   Receive a query.

   Post: the end of the query response queue will contain a response to the query
 */
BEGIN_INPUT (AUTO_PARAMETER, REGISTRY_QUERY_REQUEST_NO, REGISTRY_QUERY_REQUEST_NAME, "", query_request, aid_t aid, bd_t bda, bd_t bdb)
{
  ssyslog ("registry: query_request\n");
  initialize ();

  registry_method_t method;
  const void* specification;
  size_t size;

  if (read_registry_query_request (bda, &method, &specification, &size) == -1) {
    form_query_response (aid, REGISTRY_BAD_REQUEST, 0);
    end_action (false, bda, bdb);
  }

  /* Inspect the method. */
  switch (method) {
  case REGISTRY_STRING_EQUAL:
    /* Okay. */
    break;
  default:
    form_query_response (aid, REGISTRY_UNKNOWN_METHOD, method);
    return;
    break;
  }

  registry_query_response_t r;
  bd_t answer_bd = registry_query_response_initw (&r, REGISTRY_SUCCESS, method);

  description_t* d;
  for (d = description_head; d != 0; d = d->next) {
    if (d->method == method &&
	match (d->method, d->description, d->description_size, specification, size)) {
      /* Write a match. */
      registry_query_response_append (&r, d->aid, d->description, d->description_size);
    }
  }

  buffer_queue_push (&qr_queue, aid, answer_bd, -1);

  end_action (false, bda, bdb);
}

/* query_response
   --------------
   Relay the result of a query to a user.

   Pre:  the query response queue contains a response for the given automaton
   Post: the first response in the query response queue is removed and returned
 */
BEGIN_OUTPUT (AUTO_PARAMETER, REGISTRY_QUERY_RESPONSE_NO, REGISTRY_QUERY_RESPONSE_NAME, "", query_response, aid_t aid)
{
  ssyslog ("registry: query_response\n");
  initialize ();
  scheduler_remove (REGISTRY_QUERY_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&qr_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bda = buffer_queue_item_bda (item);
    bd_t bdb = buffer_queue_item_bdb (item);
    buffer_queue_erase (&qr_queue, item);
    end_action (true, bda, bdb);
  }
  else {
    /* Did not find a response. */
    end_action (false, -1, -1);
  }
}

/* destroyed
   ---------
   An automaton that has registered has been destroyed.
   Purge the automaton from the registry.

   Post: the list of descriptions contains no descriptions for the given aid
 */
BEGIN_SYSTEM_INPUT (REGISTRY_DESTROYED_NO, "", "", destroyed, aid_t aid, bd_t bda, bd_t bdb)
{
  ssyslog ("registry: destroyed\n");
  initialize ();

  /* The automaton corresponding to aid has been destroyed.
     Remove all relevant descriptions. */
  description_t** dp = &description_head;
  while (*dp != 0) {
    if ((*dp)->aid != aid) {
      dp = &(*dp)->next;
    }
    else {
      description_t* d = *dp;
      *dp = d->next;
      destroy_description (d);
    }
  }

  end_action (false, bda, bdb);
}

/* destroy_buffers
   ---------------
   Destroys all of the buffers in destroy_queue.
   This is useful for output actions that need to destroy the buffer *after* the output has fired.
   To schedule a buffer for destruction, just add it to destroy_queue.

   Pre:  Destroy queue is not empty.
   Post: Destroy queue is empty.
 */
static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);
  if (destroy_buffers_precondition ()) {
    /* Drain the queue. */
    while (!buffer_queue_empty (&destroy_queue)) {
      bd_t bd;
      const buffer_queue_item_t* item = buffer_queue_front (&destroy_queue);
      bd = buffer_queue_item_bda (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }
      bd = buffer_queue_item_bdb (item);
      if (bd != -1) {
	buffer_destroy (bd);
      }

      buffer_queue_pop (&destroy_queue);
    }
  }

  end_action (false, -1, -1);
}

/* end_action is a helper function for terminating actions.
   If the buffer is not -1, it schedules it to be destroyed.
   end_action schedules local actions and calls scheduler_finish to finish the action.
*/
static void
end_action (bool output_fired,
	    bd_t bda,
	    bd_t bdb)
{
  if (bda != -1|| bdb != -1) {
    buffer_queue_push (&destroy_queue, 0, bda, bdb);
  }

  if (!buffer_queue_empty (&rr_queue)) {
    scheduler_add (REGISTRY_REGISTER_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&rr_queue)));
  }
  if (!buffer_queue_empty (&qr_queue)) {
    scheduler_add (REGISTRY_QUERY_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&qr_queue)));
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }

  scheduler_finish (output_fired, bda, bdb);
}
