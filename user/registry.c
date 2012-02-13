#include "registry.h"
#include <automaton.h>
#include <io.h>
#include <string.h>
#include <buffer_heap.h>
#include <dymem.h>
#include <fifo_scheduler.h>

/*
  The Registry Automaton
  ======================
  The registry is a database of automata.
  The goal of the registry is to allow independent automata to find each other.
  For example, an automaton that manipulates files needs to find an automaton that serves those files, e.g., a virtual file system automaton.

  The two main activities involving the registry are registration and query.
  For registration, an automaton submits a description and algorithm to use when matching.
  The registry associates the description and algorithm with the automaton identifier (aid) of the registering automaton.
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

  Bugs
  ----
  * We don't account for the possibility of huge descriptions.
  * Matching can be involved.  We could move it to an internal action to increase concurrency.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

typedef struct description_struct description_t;
struct description_struct {
  aid_t aid;
  registry_method_t method;
  void* description;
  size_t description_size;
  description_t* next;
};

/* To keep things simple, we'll use a single list of descriptions.
   Feel free to use a different scheme if justified.
*/
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
description_replace_description (description_t* d,
				 const void* description,
				 size_t description_size)
{
  free (d->description);
  d->description = malloc (description_size);
  memcpy (d->description, description, description_size);
  d->description_size = description_size;
}

/* Data structure used for the response queues. */
typedef struct response_item_struct response_item_t;
struct response_item_struct {
  aid_t aid;
  bd_t bd;
  response_item_t* next;
};

/* Queue for register responses. */
static response_item_t* rr_item_head = 0;
static response_item_t** rr_item_tail = &rr_item_head;

/* Queue for query responses. */
static response_item_t* qr_item_head = 0;
static response_item_t** qr_item_tail = &qr_item_head;

static void
form_register_response (aid_t aid,
			registry_error_t error)
{
  /* Create a response. */
  bd_t bd = buffer_create (sizeof (registry_register_response_t));
  registry_register_response_t* rr = buffer_map (bd);
  rr->error = error;
  /* We don't unmap it because we are going to destroy it when we respond. */

  /* Create a queue item. */
  response_item_t* item = malloc (sizeof (response_item_t));
  memset (item, 0, sizeof (response_item_t));
  item->aid = aid;
  item->bd = bd;

  /* Insert into the queue. */
  *rr_item_tail = item;
  rr_item_tail = &item->next;
}

static void
form_query_response (aid_t aid,
		     registry_error_t error,
		     registry_method_t method)
{
  /* Create a response. */
  bd_t bd = buffer_create (sizeof (registry_register_response_t));
  registry_query_response_t* qr = buffer_map (bd);
  qr->error = error;
  qr->method = method;
  qr->result = 0;
  /* We don't unmap it because we are going to destroy it when we respond. */

  /* Create a queue item. */
  response_item_t* item = malloc (sizeof (response_item_t));
  memset (item, 0, sizeof (response_item_t));
  item->aid = aid;
  item->bd = bd;

  /* Insert into the queue. */
  *qr_item_tail = item;
  qr_item_tail = &item->next;
}

void
init (int param,
      bd_t bd,
      const char* begin,
      size_t capacity)
{
  if (set_registry () == -1) {
    const char* s = "registry: error:  Couldn't not set registry\n";
    syslog (s, strlen (s));
    exit ();
  }

  finish (NO_ACTION, 0, bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

static void
process_register (aid_t aid,
		  bd_t bd,
		  void* ptr,
		  size_t capacity)
{
  if (bd == -1) {
    form_register_response (aid, REGISTRY_NO_BUFFER);
    return;
  }

  if (ptr == 0) {
    form_register_response (aid, REGISTRY_BUFFER_TOO_BIG);
    return;
  }

  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, capacity);
  const registry_register_request_t* r = buffer_heap_begin (&heap);
  if (!buffer_heap_check (&heap, r, sizeof (registry_register_request_t))) {
    form_register_response (aid, REGISTRY_BAD_REQUEST);
    return;
  }
  const char* description = (void*)&r->description + r->description;
  if (!buffer_heap_check (&heap, description, r->description_size)) {
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

static void
process_query (aid_t aid,
	       bd_t bd,
	       void* ptr,
	       size_t capacity)
{
  if (bd == -1) {
    form_query_response (aid, REGISTRY_NO_BUFFER, 0);
    return;
  }

  if (ptr == 0) {
    form_query_response (aid, REGISTRY_BUFFER_TOO_BIG, 0);
    return;
  }

  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, capacity);
  const registry_query_request_t* q = buffer_heap_begin (&heap);
  if (!buffer_heap_check (&heap, q, sizeof (registry_query_request_t))) {
    form_query_response (aid, REGISTRY_BAD_REQUEST, 0);
    return;
  }
  const char* specification = (void*)&q->specification + q->specification;
  if (!buffer_heap_check (&heap, specification, q->specification_size)) {
    form_query_response (aid, REGISTRY_BAD_REQUEST, q->method);
    return;
  }

  /* Inspect the method. */
  switch (q->method) {
  case REGISTRY_STRING_EQUAL:
    /* TODO */
    break;
  default:
    form_query_response (aid, REGISTRY_UNKNOWN_METHOD, q->method);
    return;
    break;
  }
}

static void
schedule (void);

void
register_request (aid_t aid,
		  bd_t bd,
		  void* ptr,
		  size_t capacity)
{
  process_register (aid, bd, ptr, capacity);
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, REGISTER_REGISTER_REQUEST, register_request);

void
register_response (aid_t aid,
		   size_t bc)
{
  /* Find in the queue. */
  response_item_t** ptr;
  for (ptr = &rr_item_head; *ptr != 0; ptr = &(*ptr)->next) {
    if ((*ptr)->aid == aid) {
      break;
    }
  }

  if (*ptr != 0) {
    /* Found a response.  Execute. */
    
    /* Remove the item from the queue. */
    response_item_t* item = *ptr;
    *ptr = item->next;

    if (rr_item_head == 0) {
      rr_item_tail = &rr_item_head;
    }

    bd_t bd = item->bd;
    free (item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, REGISTER_REGISTER_RESPONSE, register_response);

void
query_request (aid_t aid,
	       bd_t bd,
	       void* ptr,
	       size_t capacity)
{
  process_query (aid, bd, ptr, capacity);
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, REGISTER_QUERY_REQUEST, query_request);

void
query_response (aid_t aid,
		size_t bc)
{
  /* Find in the queue. */
  response_item_t** ptr;
  for (ptr = &qr_item_head; *ptr != 0; ptr = &(*ptr)->next) {
    if ((*ptr)->aid == aid) {
      break;
    }
  }

  if (*ptr != 0) {
    /* Found a response.  Execute. */
    
    /* Remove the item from the queue. */
    response_item_t* item = *ptr;
    *ptr = item->next;

    if (qr_item_head == 0) {
      qr_item_tail = &qr_item_head;
    }

    bd_t bd = item->bd;
    free (item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, REGISTER_QUERY_RESPONSE, query_response);

static void
schedule (void)
{
  if (rr_item_head != 0) {
    scheduler_add (REGISTER_REGISTER_RESPONSE, rr_item_head->aid);
  }
  if (qr_item_head != 0) {
    scheduler_add (REGISTER_QUERY_RESPONSE, qr_item_head->aid);
  }
}
