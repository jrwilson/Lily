#include "registry.h"
#include <automaton.h>
#include <io.h>
#include <string.h>
#include <dymem.h>
#include <fifo_scheduler.h>
#include <buffer_queue.h>
#include <buffer_file.h>

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

static void
form_register_response (aid_t aid,
			registry_error_t error)
{
  /* Create a response. */
  bd_t bd = buffer_create (size_to_pages (sizeof (registry_register_response_t)));
  registry_register_response_t* rr = buffer_map (bd);
  rr->error = error;
  /* We don't unmap it because we are going to destroy it when we respond. */

  buffer_queue_push (&rr_queue, aid, bd);
}

static void
form_query_response (aid_t aid,
		     registry_error_t error,
		     registry_method_t method)
{
  /* Create a response. */
  bd_t bd = buffer_create (size_to_pages (sizeof (registry_register_response_t)));
  registry_query_response_t* qr = buffer_map (bd);
  qr->error = error;
  qr->method = method;
  qr->count = 0;
  /* We don't unmap it because we are going to destroy it when we respond. */

  buffer_queue_push (&qr_queue, aid, bd);
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
    buffer_queue_init (&rr_queue);
    buffer_queue_init (&qr_queue);
  }
}

static void
process_register (aid_t aid,
		  bd_t bd,
		  size_t bd_size,
		  void* ptr)
{
  if (bd == -1) {
    form_register_response (aid, REGISTRY_NO_BUFFER);
    return;
  }

  if (ptr == 0) {
    form_register_response (aid, REGISTRY_BUFFER_TOO_BIG);
    return;
  }

  buffer_file_t file;
  buffer_file_open (&file, bd, bd_size, ptr, false);

  const registry_register_request_t* r = buffer_file_readp (&file, sizeof (registry_register_request_t));
  if (r == 0) {
    form_register_response (aid, REGISTRY_BAD_REQUEST);
    return;
  }
  const void* description = buffer_file_readp (&file, r->description_size);
  if (description == 0) {
    form_register_response (aid, REGISTRY_BAD_REQUEST);
    return;
  }

  /* Check that no description is equal to the given description. */
  description_t* d;
  for (d = description_head; d != 0; d = d->next) {
    if (d->description_size == r->description_size &&
	memcmp (d->description, description, r->description_size) == 0) {
      /* Found a description that is equal. */
      break;
    }
  }
  if (d != 0) {
    form_register_response (aid, REGISTRY_NOT_UNIQUE);
    return;
  }

  /* Inspect the method. */
  switch (r->method) {
  case REGISTRY_STRING_EQUAL:
    /* Okay. */
    break;
  default:
    form_register_response (aid, REGISTRY_UNKNOWN_METHOD);
    return;
    break;
  }

  /* Insert a new description or replace an existing description. */
  for (d = description_head; d != 0; d = d->next) {
    if (d->aid == aid &&
	d->method == r->method) {
      /* Found a description for this automaton and method. */
      break;
    }
  }
  if (d == 0) {
    /* Create a new description. */
    d = create_description (aid, r->method, description, r->description_size);
    d->next = description_head;
    description_head = d;
  }
  else {
    /* Replace the existing description. */
    description_replace_description (d, description, r->description_size);
    free (d->description);
    d->description = malloc (r->description_size);
    memcpy (d->description, description, r->description_size);
    d->description_size = r->description_size;
  }
  
  form_register_response (aid, REGISTRY_SUCCESS);
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
process_query (aid_t aid,
	       bd_t bd,
	       size_t bd_size,
	       void* ptr)
{
  if (bd == -1) {
    form_query_response (aid, REGISTRY_NO_BUFFER, 0);
    return;
  }

  if (ptr == 0) {
    form_query_response (aid, REGISTRY_BUFFER_TOO_BIG, 0);
    return;
  }

  buffer_file_t file;
  buffer_file_open (&file, bd, bd_size, ptr, false);
  
  const registry_query_request_t* q = buffer_file_readp (&file, sizeof (registry_query_request_t));
  if (q == 0) {
    form_query_response (aid, REGISTRY_BAD_REQUEST, 0);
    return;
  }
  const void* specification = buffer_file_readp (&file, q->specification_size);
  if (specification == 0) {
    form_query_response (aid, REGISTRY_BAD_REQUEST, q->method);
    return;
  }

  /* Inspect the method. */
  switch (q->method) {
  case REGISTRY_STRING_EQUAL:
    /* Okay. */
    break;
  default:
    form_query_response (aid, REGISTRY_UNKNOWN_METHOD, q->method);
    return;
    break;
  }

  buffer_file_t answer;
  buffer_file_create (&answer, 0);

  /* Skip over the header. */
  buffer_file_seek (&answer, sizeof (registry_query_response_t), BUFFER_FILE_SET);

  size_t count = 0;
  description_t* d;
  for (d = description_head; d != 0; d = d->next) {
    if (d->method == q->method &&
	match (d->method, d->description, d->description_size, specification, q->specification_size)) {
      /* Write a match. */
      registry_query_result_t result;
      result.aid = d->aid;
      result.description_size = d->description_size;
      buffer_file_write (&answer, &result, sizeof (registry_query_result_t));
      buffer_file_write (&answer, d->description, d->description_size);
      ++count;
    }
  }

  /* Write the header. */
  buffer_file_seek (&answer, 0, BUFFER_FILE_SET);
  
  registry_query_response_t response;
  response.error = REGISTRY_SUCCESS;
  response.method = q->method;
  response.count = count;
  buffer_file_write (&answer, &response, sizeof (registry_query_response_t));
}

static void
schedule (void);

void
init (aid_t aid,
      bd_t bd,
      size_t bd_size,
      const void* ptr)
{
  initialize ();
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, 0, init, INIT, "", "");

void
register_request (aid_t aid,
		  bd_t bd,
		  size_t bd_size,
		  void* ptr)
{
  initialize ();
  subscribe_destroyed (aid, REGISTRY_DESTROYED_NO);
  process_register (aid, bd, bd_size, ptr);
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, AUTO_MAP, register_request, REGISTRY_REGISTER_REQUEST_NO, REGISTRY_REGISTER_REQUEST_NAME, "");

void
register_response (aid_t aid,
		   size_t bc)
{
  initialize ();
  scheduler_remove (REGISTRY_REGISTER_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&rr_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&rr_queue, item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (-1, FINISH_NOOP);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, 0, register_response, REGISTRY_REGISTER_RESPONSE_NO, REGISTRY_REGISTER_RESPONSE_NAME, "");

void
query_request (aid_t aid,
	       bd_t bd,
	       size_t bd_size,
	       void* ptr)
{
  initialize ();
  process_query (aid, bd, bd_size, ptr);
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, AUTO_MAP, query_request, REGISTRY_QUERY_REQUEST_NO, REGISTRY_QUERY_REQUEST_NAME, "");

void
query_response (aid_t aid,
		size_t bc)
{
  initialize ();
  scheduler_remove (REGISTRY_QUERY_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&qr_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&qr_queue, item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (-1, FINISH_NOOP);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, 0, query_response, REGISTRY_QUERY_RESPONSE_NO, REGISTRY_QUERY_RESPONSE_NAME, "");

void
destroyed (aid_t aid,
	   bd_t bd,
	   size_t bd_size,
	   void* ptr)
{
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

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, 0, destroyed, REGISTRY_DESTROYED_NO, "", "");

static void
schedule (void)
{
  if (!buffer_queue_empty (&rr_queue)) {
    scheduler_add (REGISTRY_REGISTER_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&rr_queue)));
  }
  if (!buffer_queue_empty (&qr_queue)) {
    scheduler_add (REGISTRY_QUERY_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&qr_queue)));
  }
}
