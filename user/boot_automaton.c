#include <automaton.h>
#include <string.h>
#include <description.h>
#include <dymem.h>
#include "cpio.h"
#include "de.h"
#include "environment.h"
#include "system.h"
#include "system_msg.h"

/*
  The Boot Automaton
  ==================
  The goal of the boot automaton is to create a minimal environment that allows automata to load other automata from a store.
  The environment consists of three optional automata:
    1.  syslog - The syslog automaton receives log events and outputs them as text.
    2.  tmpfs - The tmpfs automaton implements a simple file system based on the contents of a cpio archive.  (The buffer supplied to the boot automaton is passed to the tmpfs automaton.)
    3.  finda - The finda automaton allows automata to find each other.
  After attempting to create these automata, the boot automaton loads a shell automaton to continue the boot process.

  TODO:  Find a balance between the data structures in the kernel and the system automaton.

  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define INIT_NO 1

#define CREATE_REQUEST_IN_NO 2
#define CA_REQUEST_OUT_NO 3
#define CA_RESPONSE_IN_NO 4
#define CREATE_RESULT_OUT_NO 5
#define CREATE_RESPONSE_OUT_NO 6

#define BIND_REQUEST_IN_NO 7
#define BA_REQUEST_OUT_NO 8
#define BA_RESPONSE_IN_NO 9
#define BIND_RESULT_OUT_NO 10
#define BIND_RESPONSE_OUT_NO 11

/* Paths in the cpio archive. */
#define SYSLOG_PATH "bin/syslog"
#define TMPFS_PATH "bin/tmpfs"
#define FINDA_PATH "bin/finda"
#define JSH_PATH "bin/jsh"

/* Logging. */
#define LOG_BUFFER_SIZE 128
static char log_buffer[LOG_BUFFER_SIZE];
#define ERROR __FILE__ ": error: "
#define WARNING __FILE__ ": warning: "
#define INFO __FILE__ ": info: "
#define TODO __FILE__ ": todo: "

/* Initialization flag. */
static bool initialized = false;

/* This automaton's aid. */
static aid_t system_automaton_aid = -1;

static lily_create_error_t create_error;

static aid_t
create (bd_t text_bd,
	bd_t bda,
	bd_t bdb,
	int retain_privilege)
{
  aid_t retval;
  syscall4re (LILY_SYSCALL_CREATE, retval, text_bd, bda, retain_privilege, retval);
  return retval;
}

static lily_bind_error_t bind_error;

static bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      int output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      int input_parameter)
{
  bid_t retval;
  __asm__ ("push %8\n"
	   "push %7\n"
	   "push %6\n"
	   "push %5\n"
	   "push %4\n"
	   "push %3\n"
	   "mov %2, %%eax\n"
	   "int $0x80\n"
	   "mov %%eax, %0\n"
	   "mov %%ecx, %1\n"
	   "sub $24, %%esp\n" : "=g"(retval), "=g"(bind_error) : "g"(LILY_SYSCALL_BIND), "g"(output_automaton), "g"(output_action), "g"(output_parameter), "g"(input_automaton), "g"(input_action), "g"(input_parameter) : "eax", "ecx");

  return retval;
}

static int
unbind (bid_t bid)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNBIND, retval, bid);
  return retval;
}

static int
destroy (aid_t aid)
{
  int retval;
  syscall1re (LILY_SYSCALL_DESTROY, retval, aid);
  return retval;
}

static int
exists (aid_t aid)
{
  int retval;
  syscall1r (LILY_SYSCALL_EXISTS, aid, retval);
  return retval;
}

static size_t
binding_count (ano_t ano,
	       int parameter)
{
  size_t retval;
  syscall2r (LILY_SYSCALL_BINDING_COUNT, ano, parameter, retval);
  return retval;
}

static aid_t
create_ (bd_t text_bd,
	 bd_t bda,
	 bd_t bdb,
	 int retain_privilege)
{
  /* Create the automaton. */
  aid_t aid = create (text_bd, bda, bdb, retain_privilege);


  if (aid != -1) {
    /* Bind it to the system automaton. */
    description_t description;
    action_desc_t desc;
    description_init (&description, aid);

    if (description_read_name (&description, &desc, SA_CREATE_REQUEST_OUT_NAME, 0) == 0 && desc.type == LILY_ACTION_OUTPUT) {
      bind (aid, desc.number, 0, system_automaton_aid, CREATE_REQUEST_IN_NO, aid);
    }
    if (description_read_name (&description, &desc, SA_CA_REQUEST_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, CA_REQUEST_OUT_NO, aid, aid, desc.number, 0);
    }
    if (description_read_name (&description, &desc, SA_CA_RESPONSE_OUT_NAME, 0) == 0 && desc.type == LILY_ACTION_OUTPUT) {
      bind (aid, desc.number, 0, system_automaton_aid, CA_RESPONSE_IN_NO, aid);
    }
    if (description_read_name (&description, &desc, SA_CREATE_RESULT_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, CREATE_RESULT_OUT_NO, aid, aid, desc.number, 0);
    }
    if (description_read_name (&description, &desc, SA_CREATE_RESPONSE_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, CREATE_RESPONSE_OUT_NO, aid, aid, desc.number, 0);
    }

    if (description_read_name (&description, &desc, SA_BIND_REQUEST_OUT_NAME, 0) == 0 && desc.type == LILY_ACTION_OUTPUT) {
      bind (aid, desc.number, 0, system_automaton_aid, BIND_REQUEST_IN_NO, aid);
    }
    if (description_read_name (&description, &desc, SA_BA_REQUEST_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, BA_REQUEST_OUT_NO, aid, aid, desc.number, 0);
    }
    if (description_read_name (&description, &desc, SA_BA_RESPONSE_OUT_NAME, 0) == 0 && desc.type == LILY_ACTION_OUTPUT) {
      bind (aid, desc.number, 0, system_automaton_aid, BA_RESPONSE_IN_NO, aid);
    }
    if (description_read_name (&description, &desc, SA_BIND_RESULT_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, BIND_RESULT_OUT_NO, aid, aid, desc.number, 0);
    }
    if (description_read_name (&description, &desc, SA_BIND_RESPONSE_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, BIND_RESPONSE_OUT_NO, aid, aid, desc.number, 0);
    }

  }
  return aid;
}

/* Create Protocol
   ---------------
   The create protocol involves three parties:  the initiator, the system automaton, and the owner automaton (optional).
   A typical execution is depicted as follows:
     Message       Initiator   System   Owner
     create_request    |--------->|       |
     ca_request        |          |------>|
     ca_response       |          |<------|
                               create!
     create_result     |          |------>|
     create_response   |<---------|       |

   ca stands for create authorization.
   The owner automaton can prevent the create from succeeding.
   Seek system_msg.h for the contents of each message.
 */
typedef struct create_result create_result_t;
struct create_result {
  aid_t to;
  sa_create_result_t result;
  create_result_t* next;
};

static create_result_t* create_result_head = 0;
static create_result_t** create_result_tail = &create_result_head;

static void
push_create_result (aid_t to,
		    sa_create_outcome_t outcome)
{
  create_result_t* res = malloc (sizeof (create_result_t));
  memset (res, 0, sizeof (create_result_t));
  res->to = to;
  sa_create_result_init (&res->result, outcome);
  
  *create_result_tail = res;
  create_result_tail = &res->next;
}

static void
pop_create_result (void)
{
  create_result_t* res = create_result_head;
  create_result_head = res->next;
  if (create_result_head == 0) {
    create_result_tail = &create_result_head;
  }
  free (res);
}

typedef struct create_response create_response_t;
struct create_response {
  aid_t to;
  sa_create_response_t response;
  create_response_t* next;
};

static create_response_t* create_response_head = 0;
static create_response_t** create_response_tail = &create_response_head;

static void
push_create_response (aid_t to,
		      sa_create_outcome_t outcome,
		      aid_t aid)
{
  create_response_t* res = malloc (sizeof (create_response_t));
  memset (res, 0, sizeof (create_response_t));
  res->to = to;
  sa_create_response_init (&res->response, outcome, aid);
  
  *create_response_tail = res;
  create_response_tail = &res->next;
}

static void
pop_create_response (void)
{
  create_response_t* res = create_response_head;
  create_response_head = res->next;
  if (create_response_head == 0) {
    create_response_tail = &create_response_head;
  }
  free (res);
}

typedef enum {
  UNKNOWN = 0,
  DENIED,
  GRANTED,
} approval_t;

typedef struct create_transaction create_transaction_t;
struct create_transaction {
  sa_create_request_t* request;
  mono_time_t time; /* A timestamp so requests can timeout. */
  approval_t owner_approval;
  aid_t initiator_aid;
  create_transaction_t* next;
};

static create_transaction_t* create_transaction_head = 0;
static create_transaction_t** create_transaction_tail = &create_transaction_head;

static create_transaction_t*
insert_create_transaction (sa_create_request_t* request,
			   aid_t initiator_aid)
{
  create_transaction_t* req = malloc (sizeof (create_transaction_t));
  memset (req, 0, sizeof (create_transaction_t));
  req->request = request;
  getmonotime (&req->time);
  req->initiator_aid = initiator_aid;

  *create_transaction_tail = req;
  create_transaction_tail = &req->next;

  return req;
}

static create_transaction_t*
find_create_transaction (void)
{
  logs (__func__);
  return create_transaction_head;
  /* TODO */
  /* for (create_transaction_t* ptr = create_transaction_head; ptr != 0; ptr = ptr->next) { */
  /*   if (sa_createing_equal (&ptr->createing, b)) { */
  /*     return ptr; */
  /*   } */
  /* } */
  return 0;
}

static void
remove_create_transaction (create_transaction_t* t)
{
  for (create_transaction_t** ptr = &create_transaction_head; *ptr != 0; ptr = &(*ptr)->next) {
    if (*ptr == t) {
      *ptr = t->next;
      sa_create_request_destroy (t->request);
      free (t);
      break;
    }
  }
}

static void
process_create_transaction (create_transaction_t* t)
{
  /* Wait until all of the parties have answered.
     This means there are no outstanding references to the transaction so we can destroy it.
     If we don't wait, we could send a result before sending a request.
     This may or may not be a problem.
     It is my personal opinion that waiting makes the protocol easier to understand.
  */
  if (t->owner_approval != UNKNOWN) {
    sa_create_outcome_t outcome = SA_CREATE_NOT_AUTHORIZED;
    const sa_create_request_t* req = t->request;
    aid_t aid = -1;

    if (t->owner_approval == GRANTED) {
      /* Permission granted. */
      aid = create_ (req->text_bd, req->bda, req->bdb, req->retain_privilege);
      switch (create_error) {
      case LILY_CREATE_ERROR_SUCCESS:
      	outcome = SA_CREATE_SUCCESS;
      	break;
      case LILY_CREATE_ERROR_PERMISSION:
      	/* Should not happen. */
      	break;
      case LILY_CREATE_ERROR_INVAL:
      	outcome = SA_CREATE_INVAL;
      	break;
      case LILY_CREATE_ERROR_BDDNE:
      	outcome = SA_CREATE_BDDNE;
      	break;
      }
    }

    /* Send the result. */
    push_create_result (req->owner_aid, outcome);
    push_create_response (t->initiator_aid, outcome, aid);

    remove_create_transaction (t);
  }
}

typedef struct ca_request ca_request_t;
struct ca_request {
  aid_t to;
  create_transaction_t* transaction;
  sa_ca_request_t request;
  ca_request_t* next;
};

static ca_request_t* ca_request_head = 0;
static ca_request_t** ca_request_tail = &ca_request_head;

static void
push_ca_request (aid_t to,
		 create_transaction_t* transaction)
{
  ca_request_t* rfa = malloc (sizeof (ca_request_t));
  memset (rfa, 0, sizeof (ca_request_t));
  rfa->to = to;
  rfa->transaction = transaction;
  sa_ca_request_init (&rfa->request);
  
  *ca_request_tail = rfa;
  ca_request_tail = &rfa->next;
}

static void
pop_ca_request (void)
{
  ca_request_t* rfa = ca_request_head;
  ca_request_head = rfa->next;
  if (ca_request_head == 0) {
    ca_request_tail = &ca_request_head;
  }
  free (rfa);
}

/* Bind Protocol
   -------------
   The bind protocol involves five parties:  the initiator, the system automaton, the output automaton, the input automaton, and the owner automaton (optional).
   A typical execution is depicted as follows:
     Message       Initiator   System   Output   Input   Owner
     bind_request      |--------->|        |       |       |
     ba_request        |          |------->|       |       |
     ba_request        |          |--------|------>|       |
     ba_request        |          |--------|-------|------>|
     ba_response       |          |<-------|       |       |
     ba_response       |          |<-------|-------|       |
     ba_response       |          |<-------|-------|-------|
                                bind!
     bind_result       |          |------->|       |       |
     bind_result       |          |--------|------>|       |
     bind_result       |          |--------|-------|------>|
     bind_response     |<---------|        |       |       |

   ba stands for bind authorization.
   Thus, the output, input, or owner automaton can prevent the binding from succeeding.
   Seek system_msg.h for the contents of each message.
 */
typedef struct bind_result bind_result_t;
struct bind_result {
  aid_t to;
  sa_bind_result_t result;
  bind_result_t* next;
};

static bind_result_t* bind_result_head = 0;
static bind_result_t** bind_result_tail = &bind_result_head;

static void
push_bind_result (aid_t to,
		  const sa_binding_t* binding,
		  sa_bind_role_t role,
		  sa_bind_outcome_t outcome)
{
  bind_result_t* res = malloc (sizeof (bind_result_t));
  memset (res, 0, sizeof (bind_result_t));
  res->to = to;
  sa_bind_result_init (&res->result, binding, role, outcome);
  
  *bind_result_tail = res;
  bind_result_tail = &res->next;
}

static void
pop_bind_result (void)
{
  bind_result_t* res = bind_result_head;
  bind_result_head = res->next;
  if (bind_result_head == 0) {
    bind_result_tail = &bind_result_head;
  }
  free (res);
}

typedef struct bind_response bind_response_t;
struct bind_response {
  aid_t to;
  sa_bind_response_t response;
  bind_response_t* next;
};

static bind_response_t* bind_response_head = 0;
static bind_response_t** bind_response_tail = &bind_response_head;

static void
push_bind_response (aid_t to,
		    const sa_binding_t* binding,
		    sa_bind_outcome_t outcome)
{
  bind_response_t* res = malloc (sizeof (bind_response_t));
  memset (res, 0, sizeof (bind_response_t));
  res->to = to;
  sa_bind_response_init (&res->response, binding, outcome);
  
  *bind_response_tail = res;
  bind_response_tail = &res->next;
}

static void
pop_bind_response (void)
{
  bind_response_t* res = bind_response_head;
  bind_response_head = res->next;
  if (bind_response_head == 0) {
    bind_response_tail = &bind_response_head;
  }
  free (res);
}

typedef struct bind_transaction bind_transaction_t;
struct bind_transaction {
  sa_binding_t binding;
  mono_time_t time; /* A timestamp so requests can timeout. */
  approval_t output_approval;
  approval_t input_approval;
  approval_t owner_approval;
  aid_t initiator_aid;
  bind_transaction_t* next;
};

static bind_transaction_t* bind_transaction_head = 0;
static bind_transaction_t** bind_transaction_tail = &bind_transaction_head;

static bind_transaction_t*
insert_bind_transaction (const sa_binding_t* b,
			 aid_t initiator_aid)
{
  bind_transaction_t* req = malloc (sizeof (bind_transaction_t));
  memset (req, 0, sizeof (bind_transaction_t));
  req->binding = *b;
  getmonotime (&req->time);
  req->initiator_aid = initiator_aid;

  *bind_transaction_tail = req;
  bind_transaction_tail = &req->next;

  return req;
}

static bind_transaction_t*
find_bind_transaction (const sa_binding_t* b)
{
  for (bind_transaction_t* ptr = bind_transaction_head; ptr != 0; ptr = ptr->next) {
    if (sa_binding_equal (&ptr->binding, b)) {
      return ptr;
    }
  }
  return 0;
}

static void
remove_bind_transaction (bind_transaction_t* t)
{
  for (bind_transaction_t** ptr = &bind_transaction_head; *ptr != 0; ptr = &(*ptr)->next) {
    if (*ptr == t) {
      *ptr = t->next;
      free (t);
      break;
    }
  }
}

static void
process_bind_transaction (bind_transaction_t* t)
{
  /* Wait until all of the parties have answered.
     This means there are no outstanding references to the transaction so we can destroy it.
     If we don't wait, we could send a result before sending a request.
     This may or may not be a problem.
     It is my personal opinion that waiting makes the protocol easier to understand.
  */
  if (t->output_approval != UNKNOWN &&
      t->input_approval != UNKNOWN &&
      t->owner_approval != UNKNOWN) {
    const sa_binding_t* b = &t->binding;
    sa_bind_outcome_t outcome = SA_BIND_NOT_AUTHORIZED;

    if (t->output_approval == GRANTED &&
	t->input_approval == GRANTED &&
	t->owner_approval == GRANTED) {
      /* Permission granted. */
      bind (b->output_aid, b->output_ano, b->output_parameter, b->input_aid, b->input_ano, b->input_parameter);
      switch (bind_error) {
      case LILY_BIND_ERROR_SUCCESS:
	outcome = SA_BIND_SUCCESS;
	break;
      case LILY_BIND_ERROR_PERMISSION:
	/* Should not happen. */
	break;
      case LILY_BIND_ERROR_OAIDDNE:
	outcome = SA_BIND_OAIDDNE;
	break;
      case LILY_BIND_ERROR_IAIDDNE:
	outcome = SA_BIND_IAIDDNE;
	break;
      case LILY_BIND_ERROR_OANODNE:
	outcome = SA_BIND_OANODNE;
	break;
      case LILY_BIND_ERROR_IANODNE:
	outcome = SA_BIND_IANODNE;
	break;
      case LILY_BIND_ERROR_SAME:
	outcome = SA_BIND_SAME;
	break;
      case LILY_BIND_ERROR_ALREADY:
	outcome = SA_BIND_ALREADY;
	break;
      }
    }

    /* Send the result. */
    push_bind_result (b->output_aid, b, SA_BIND_OUTPUT, outcome);
    push_bind_result (b->input_aid, b, SA_BIND_INPUT, outcome);
    push_bind_result (b->owner_aid, b, SA_BIND_OWNER, outcome);
    push_bind_response (t->initiator_aid, b, outcome);

    remove_bind_transaction (t);
  }
}

typedef struct ba_request ba_request_t;
struct ba_request {
  aid_t to;
  bind_transaction_t* transaction;
  sa_ba_request_t request;
  ba_request_t* next;
};

static ba_request_t* ba_request_head = 0;
static ba_request_t** ba_request_tail = &ba_request_head;

static void
push_ba_request (aid_t to,
		 bind_transaction_t* transaction,
		 sa_bind_role_t role)
{
  ba_request_t* rfa = malloc (sizeof (ba_request_t));
  memset (rfa, 0, sizeof (ba_request_t));
  rfa->to = to;
  rfa->transaction = transaction;
  sa_ba_request_init (&rfa->request, &transaction->binding, role);
  
  *ba_request_tail = rfa;
  ba_request_tail = &rfa->next;
}

static void
pop_ba_request (void)
{
  ba_request_t* rfa = ba_request_head;
  ba_request_head = rfa->next;
  if (ba_request_head == 0) {
    ba_request_tail = &ba_request_head;
  }
  free (rfa);
}

/* Output buffer. */
static bd_t output_bda = -1;
static bd_t output_bdb = -1;
static buffer_file_t output_bfa;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    output_bda = buffer_create (0);
    output_bdb = buffer_create (0);
    if (output_bda == -1 || output_bdb == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create output buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_file_initw (&output_bfa, output_bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize output buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    system_automaton_aid = getaid ();

    bd_t bda = getinita ();
    bd_t bdb = getinitb ();

    cpio_archive_t archive;
    if (cpio_archive_init (&archive, bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize cpio archive: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    aid_t finda_aid = -1;
    aid_t tmpfs_aid = -1;
    bd_t jsh_bd = -1;
    cpio_file_t file;
    while (cpio_archive_read (&archive, &file) == 0) {
      if ((file.mode & CPIO_TYPE_MASK) == CPIO_REGULAR) {
	if (strcmp (file.name, SYSLOG_PATH) == 0) {
	  bd_t syslog_bd = cpio_file_read (&archive, &file);
	  if (syslog_bd == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read syslog image: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  aid_t syslog_aid = create_ (syslog_bd, -1, -1, false);
	  if (syslog_aid == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create syslog automaton: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  if (buffer_destroy (syslog_bd) != 0) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy syslog image: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "syslog = %d", syslog_aid);
	  logs (log_buffer);
	}
	else if (strcmp (file.name, TMPFS_PATH) == 0) {
	  bd_t tmpfs_bd = cpio_file_read (&archive, &file);
	  if (tmpfs_bd == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read tmpfs image: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  tmpfs_aid = create_ (tmpfs_bd, bda, -1, false);
	  if (tmpfs_aid == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create tmpfs automaton: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  if (buffer_destroy (tmpfs_bd) != 0) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy tmpfs image: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "tmpfs = %d", tmpfs_aid);
	  logs (log_buffer);
	}
	else if (strcmp (file.name, FINDA_PATH) == 0) {
	  bd_t finda_bd = cpio_file_read (&archive, &file);
	  if (finda_bd == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not read finda image: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  finda_aid = create_ (finda_bd, -1, -1, false);
	  if (finda_aid == -1) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create finda automaton: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  if (buffer_destroy (finda_bd) != 0) {
	    snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy finda image: %s", lily_error_string (lily_error));
	    logs (log_buffer);
	    exit (-1);
	  }
	  snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "finda = %d", finda_aid);
	  logs (log_buffer);
	}
	else if (strcmp (file.name, JSH_PATH) == 0) {
	  jsh_bd = cpio_file_read (&archive, &file);
	}
      }
    }

    if (bda != -1) {
      if (buffer_destroy (bda) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }
    if (bdb != -1) {
      if (buffer_destroy (bdb) != 0) {
    	snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy init buffer: %s", lily_error_string (lily_error));
    	logs (log_buffer);
    	exit (-1);
      }
    }

    bd_t de_bd = buffer_create (0);
    if (de_bd == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create de buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    buffer_file_t de_buffer;
    if (buffer_file_initw (&de_buffer, de_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize de buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    de_val_t* root = de_create_object ();
    de_set (root, "." FINDA, de_create_integer (finda_aid));
    de_set (root, "." FS "[0].aid", de_create_integer (tmpfs_aid));
    de_set (root, "." FS "[0].name", de_create_string ("bootfs"));
    de_set (root, "." FS "[0].nodeid", de_create_integer (0));
    de_set (root, "." ACTIVE_FS, de_create_integer (tmpfs_aid));
    de_set (root, "." ARGS "." "script", de_create_string ("/scr/start.jsh"));
    de_serialize (root, &de_buffer);
    de_destroy (root);

    if (jsh_bd == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "archive does not contain %s", JSH_PATH);
      logs (log_buffer);
      exit (-1);
    }
    aid_t jsh_aid = create_ (jsh_bd, de_bd, -1, true);
    if (jsh_aid == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create jsh automaton: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_destroy (jsh_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy jsh image: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    if (buffer_destroy (de_bd) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not destroy de buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }

    snprintf (log_buffer, LOG_BUFFER_SIZE, INFO "jsh = %d", jsh_aid);
    logs (log_buffer);
  }
}

BEGIN_INTERNAL (NO_PARAMETER, INIT_NO, "init", "", init, ano_t ano, int param)
{
  initialize ();
  finish_internal ();
}

BEGIN_INPUT (AUTO_PARAMETER, CREATE_REQUEST_IN_NO, "", "", create_request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bfa;
  if (buffer_file_initr (&bfa, bda) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not initialize create request buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  sa_create_request_t* req = sa_create_request_read (&bfa, bdb);
  if (req == 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read create request: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  /* Make a record of the create request. */
  create_transaction_t* t = insert_create_transaction (req, aid);
      
  /* Send a request for authorization to the owner. */
  push_ca_request (req->owner_aid, t);

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, CA_REQUEST_OUT_NO, "", "", ca_request_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (ca_request_head != 0 && ca_request_head->to == aid) {
    if (exists (aid) && binding_count (CA_REQUEST_OUT_NO, aid) == 1 && binding_count (CA_RESPONSE_IN_NO, aid) == 1) {
      // The automaton in question is alive and can receive/send create rfa's.
      
      // Shred the output buffer.
      buffer_file_shred (&output_bfa);

      // Write the request.
      buffer_file_write (&output_bfa, &ca_request_head->request, sizeof (sa_ca_request_t));

      // Pop the request.
      pop_ca_request ();

      finish_output (true, output_bda, -1);
    }
    else {
      // The automaton in question is either dead or can't receive/send create rfa's.
      // We interpret silence as being permissive.
      ca_request_head->transaction->owner_approval = GRANTED;
      process_create_transaction (ca_request_head->transaction);
      pop_ca_request ();
      finish_output (false, -1, -1);
    }
  }

  finish_output (false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, CA_RESPONSE_IN_NO, "", "", ca_response_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bfa;
  if (buffer_file_initr (&bfa, bda) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not initialize ca response buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  const sa_ca_response_t* res = buffer_file_readp (&bfa, sizeof (sa_ca_response_t));
  if (res == 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read ca response: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  /* Find the transaction. */
  create_transaction_t* t = find_create_transaction ();
  if (t != 0) {
    /* Check that they are authorized to answer for their role and that no answer has been given. */
    if (t->request->owner_aid == aid && t->owner_approval == UNKNOWN) {
      t->owner_approval = res->authorized ? GRANTED : DENIED;
      process_create_transaction (t);
    }
  }

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, CREATE_RESULT_OUT_NO, "", "", create_result_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (create_result_head != 0 && create_result_head->to == aid) {
    // Shred the output buffer.
    buffer_file_shred (&output_bfa);
    
    // Write the result.
    buffer_file_write (&output_bfa, &create_result_head->result, sizeof (sa_create_result_t));
    
    // Pop the result..
    pop_create_result ();
    
    finish_output (true, output_bda, -1);
  }
    
  finish_output (false, -1, -1);
}

BEGIN_OUTPUT (AUTO_PARAMETER, CREATE_RESPONSE_OUT_NO, "", "", create_response_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (create_response_head != 0 && create_response_head->to == aid) {
    // Shred the output buffer.
    buffer_file_shred (&output_bfa);
    
    // Write the response.
    buffer_file_write (&output_bfa, &create_response_head->response, sizeof (sa_create_response_t));
    
    // Pop the response..
    pop_create_response ();
    
    finish_output (true, output_bda, -1);
  }
    
  finish_output (false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, BIND_REQUEST_IN_NO, "", "", bind_request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bfa;
  if (buffer_file_initr (&bfa, bda) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not initialize bind request buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  const sa_bind_request_t* req = buffer_file_readp (&bfa, sizeof (sa_bind_request_t));
  if (req == 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read bind request: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  /* Make a record of the bind request. */
  bind_transaction_t* t = insert_bind_transaction (&req->binding, aid);
      
  /* Send a request for authorization to the three parties. */
  push_ba_request (req->binding.output_aid, t, SA_BIND_OUTPUT);
  push_ba_request (req->binding.input_aid, t, SA_BIND_INPUT);
  push_ba_request (req->binding.owner_aid, t, SA_BIND_OWNER);

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, BA_REQUEST_OUT_NO, "", "", ba_request_out, ano_t ano, aid_t aid)
{
  initialize ();
  
  if (ba_request_head != 0 && ba_request_head->to == aid) {
    if (exists (aid) && binding_count (BA_REQUEST_OUT_NO, aid) == 1 && binding_count (BA_RESPONSE_IN_NO, aid) == 1) {
      // The automaton in question is alive and can receive/send bind rfa's.
      
      // Shred the output buffer.
      buffer_file_shred (&output_bfa);

      // Write the request.
      buffer_file_write (&output_bfa, &ba_request_head->request, sizeof (sa_ba_request_t));

      // Pop the request.
      pop_ba_request ();

      finish_output (true, output_bda, -1);
    }
    else {
      // The automaton in question is either dead or can't receive/send bind rfa's.
      // We interpret silence as being permissive.
      switch (ba_request_head->request.role) {
      case SA_BIND_INPUT:
	ba_request_head->transaction->input_approval = GRANTED;
	break;
      case SA_BIND_OUTPUT:
	ba_request_head->transaction->output_approval = GRANTED;
	break;
      case SA_BIND_OWNER:
	ba_request_head->transaction->owner_approval = GRANTED;
	break;
      }
      process_bind_transaction (ba_request_head->transaction);
      pop_ba_request ();
      finish_output (false, -1, -1);
    }
  }

  finish_output (false, -1, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, BA_RESPONSE_IN_NO, "", "", ba_response_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bfa;
  if (buffer_file_initr (&bfa, bda) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not initialize ba response buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  const sa_ba_response_t* res = buffer_file_readp (&bfa, sizeof (sa_ba_response_t));
  if (res == 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read ba response: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  /* Find the transaction. */
  bind_transaction_t* t = find_bind_transaction (&res->binding);
  if (t != 0) {
    /* Check that they are authorized to answer for their role and that no answer has been given. */
    switch (res->role) {
    case SA_BIND_INPUT:
      if (t->binding.input_aid == aid && t->input_approval == UNKNOWN) {
	t->input_approval = res->authorized ? GRANTED : DENIED;
	process_bind_transaction (t);
      }
      break;
    case SA_BIND_OUTPUT:
      if (t->binding.output_aid == aid && t->output_approval == UNKNOWN) {
	t->output_approval = res->authorized ? GRANTED : DENIED;
	process_bind_transaction (t);
      }
      break;
    case SA_BIND_OWNER:
      if (t->binding.owner_aid == aid && t->owner_approval == UNKNOWN) {
	t->owner_approval = res->authorized ? GRANTED : DENIED;
	process_bind_transaction (t);
      }
      break;
    }
  }

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, BIND_RESULT_OUT_NO, "", "", bind_result_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (bind_result_head != 0 && bind_result_head->to == aid) {
    // Shred the output buffer.
    buffer_file_shred (&output_bfa);
    
    // Write the result.
    buffer_file_write (&output_bfa, &bind_result_head->result, sizeof (sa_bind_result_t));
    
    // Pop the result..
    pop_bind_result ();
    
    finish_output (true, output_bda, -1);
  }
    
  finish_output (false, -1, -1);
}

BEGIN_OUTPUT (AUTO_PARAMETER, BIND_RESPONSE_OUT_NO, "", "", bind_response_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (bind_response_head != 0 && bind_response_head->to == aid) {
    // Shred the output buffer.
    buffer_file_shred (&output_bfa);
    
    // Write the response.
    buffer_file_write (&output_bfa, &bind_response_head->response, sizeof (sa_bind_response_t));
    
    // Pop the response..
    pop_bind_response ();
    
    finish_output (true, output_bda, -1);
  }
    
  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (ca_request_head != 0) {
    schedule (CA_REQUEST_OUT_NO, ca_request_head->to);
  }
  if (create_result_head != 0) {
    schedule (CREATE_RESULT_OUT_NO, create_result_head->to);
  }
  if (create_response_head != 0) {
    schedule (CREATE_RESPONSE_OUT_NO, create_response_head->to);
  }

  if (ba_request_head != 0) {
    schedule (BA_REQUEST_OUT_NO, ba_request_head->to);
  }
  if (bind_result_head != 0) {
    schedule (BIND_RESULT_OUT_NO, bind_result_head->to);
  }
  if (bind_response_head != 0) {
    schedule (BIND_RESPONSE_OUT_NO, bind_response_head->to);
  }
}
