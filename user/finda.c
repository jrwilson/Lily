#include "finda_msg.h"
#include <automaton.h>
#include <stdbool.h>
#include <dymem.h>
#include <string.h>

/* TODO:  Receive destroyed events to clean up client list. */

#define INIT_NO 1
#define SEND_IN_NO 2
#define RECV_OUT_NO 3

static bool initialized = false;

static bd_t recv_bda = -1;
static bd_t recv_bdb = -1;

/* Reference counted buffer. */
typedef struct {
  size_t refcount;
  bd_t bda;
  bd_t bdb;
} rcb_t;

static rcb_t*
rcb_create (bd_t bda,
	    bd_t bdb)
{
  rcb_t* rcb = malloc (sizeof (rcb_t));
  memset (rcb, 0, sizeof (rcb_t));
  rcb->bda = buffer_copy (bda);
  rcb->bdb = buffer_copy (bdb);
  return rcb;
}

static void
rcb_incref (rcb_t* rcb)
{
  ++rcb->refcount;
}

static void
rcb_decref (rcb_t* rcb)
{
  --rcb->refcount;
  if (rcb->refcount == 0) {
    buffer_destroy (rcb->bda);
    buffer_destroy (rcb->bdb);
    free (rcb);
  }
}

/* Outgoing message queue. */
typedef struct msg msg_t;
struct msg {
  aid_t to;
  rcb_t* rcb;
  msg_t* next;
};

static msg_t* msg_head = 0;
static msg_t** msg_tail = &msg_head;

static void
push (aid_t to,
      rcb_t* rcb)
{
  msg_t* m = malloc (sizeof (msg_t));
  memset (m, 0, sizeof (msg_t));
  m->to = to;
  m->rcb = rcb;
  rcb_incref (rcb);
  *msg_tail = m;
  msg_tail = &m->next;
}

static void
pop (void)
{
  msg_t* m = msg_head;
  msg_head = m->next;
  if (msg_head == 0) {
    msg_tail = &msg_head;
  }

  rcb_decref (m->rcb);
  free (m);
}

/* List of clients. */
static aid_t* clients = 0;
static size_t clients_size = 0;
static size_t clients_capacity = 0;

static void
client_add (aid_t aid)
{
  for (size_t idx = 0; idx != clients_size; ++idx) {
    if (clients[idx] == aid) {
      /* Client exists. */
      return;
    }
  }

  /* Client does not exist. */
  if (clients_size == clients_capacity) {
    clients_capacity = clients_capacity * 2 + 1;
    clients = realloc (clients, clients_capacity * sizeof (aid_t));
  }
  clients[clients_size++] = aid;
}

static void
send_to_all (rcb_t* rcb)
{
  for (size_t idx = 0; idx != clients_size; ++idx) {
    push (clients[idx], rcb);
  }
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    recv_bda = buffer_create (0);
    recv_bdb = buffer_create (0);
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (AUTO_PARAMETER, SEND_IN_NO, FINDA_SEND_NAME, "", send_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();
  client_add (aid);
  rcb_t* rcb = rcb_create (bda, bdb);
  rcb_incref (rcb);
  send_to_all (rcb);
  rcb_decref (rcb);
  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, RECV_OUT_NO, FINDA_RECV_NAME, "", recv_out, ano_t ano, aid_t aid)
{
  initialize ();
  
  if (msg_head != 0 && msg_head->to == aid) {
    buffer_assign (recv_bda, msg_head->rcb->bda);
    buffer_assign (recv_bdb, msg_head->rcb->bdb);
    pop ();
    finish_output (true, recv_bda, recv_bdb);
  }

  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (msg_head != 0) {
    schedule (RECV_OUT_NO, msg_head->to);
  }
}
