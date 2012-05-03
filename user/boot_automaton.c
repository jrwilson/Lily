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
#define REQUEST_IN_NO 2
#define BIND_RFA_OUT_NO 3
#define RESPONSE_OUT_NO 4

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

typedef enum {
  UNKNOWN = 0,
  DENIED,
  GRANTED,
} approval_t;

/* Bind request processing. */
typedef struct bind_request bind_request_t;
struct bind_request {
  bind_t bind;
  mono_time_t time; /* A timestamp so requests can timeout. */
  approval_t output_approval;
  approval_t input_approval;
  approval_t owner_approval;
  bind_request_t* next;
};

static bind_request_t* bind_request_head = 0;
static bind_request_t** bind_request_tail = &bind_request_head;

static bind_request_t*
push_bind_request (const bind_t* b)
{
  bind_request_t* req = malloc (sizeof (bind_request_t));
  memset (req, 0, sizeof (bind_request_t));
  memcpy (&req->bind, b, sizeof (bind_t));
  getmonotime (&req->time);

  *bind_request_tail = req;
  bind_request_tail = &req->next;

  return req;
}

static void
bind_request_process (bind_request_t* req)
{
  logs (__func__);
}

typedef struct bind_rfa bind_rfa_t;
struct bind_rfa {
  aid_t to;
  bind_request_t* request;
  approval_t* approval;
  bind_rfa_t* next;
};

static bind_rfa_t* bind_rfa_head = 0;
static bind_rfa_t** bind_rfa_tail = &bind_rfa_head;

static void
push_bind_rfa (aid_t to,
	       bind_request_t* req,
	       approval_t* approval)
{
  bind_rfa_t* rfa = malloc (sizeof (bind_rfa_t));
  memset (rfa, 0, sizeof (bind_rfa_t));
  rfa->to = to;
  rfa->request = req;
  rfa->approval = approval;
  
  *bind_rfa_tail = rfa;
  bind_rfa_tail = &rfa->next;
}

static void
pop_bind_rfa (void)
{
  bind_rfa_t* rfa = bind_rfa_head;
  bind_rfa_head = rfa->next;
  if (bind_rfa_head == 0) {
    bind_rfa_tail = &bind_rfa_head;
  }
  free (rfa);
}

/* Output buffer. */
static bd_t output_bda = -1;
static bd_t output_bdb = -1;
static buffer_file_t output_buffer;













typedef struct result result_t;
struct result {
  aid_t aid;
  int retval;
  lily_error_t error;
  result_t* next;
};

static result_t* result_head = 0;
static result_t** result_tail = &result_head;

static void
push_result (aid_t aid,
	     int retval,
	     lily_error_t error)
{
  result_t* res = malloc (sizeof (result_t));
  memset (res, 0, sizeof (result_t));
  res->aid = aid;
  res->retval = retval;
  res->error = error;
  *result_tail = res;
  result_tail = &res->next;
}

static void
pop_result (void)
{
  result_t* res = result_head;
  result_head = res->next;
  if (result_head == 0) {
    result_tail = &result_head;
  }

  free (res);
}





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

static bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      int output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      int input_parameter)
{
  aid_t retval;
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
	   "sub $24, %%esp\n" : "=g"(retval), "=g"(lily_error) : "g"(LILY_SYSCALL_BIND), "g"(output_automaton), "g"(output_action), "g"(output_parameter), "g"(input_automaton), "g"(input_action), "g"(input_parameter) : "eax", "ecx");

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
    if (description_read_name (&description, &desc, SYSTEM_REQUEST_OUT_NAME, 0) == 0 && desc.type == LILY_ACTION_OUTPUT) {
      bind (aid, desc.number, 0, system_automaton_aid, REQUEST_IN_NO, 0);
    }
    if (description_read_name (&description, &desc, SYSTEM_BIND_RFA_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, BIND_RFA_OUT_NO, 0, aid, desc.number, 0);
    }
    if (description_read_name (&description, &desc, SYSTEM_RESPONSE_IN_NAME, 0) == 0 && desc.type == LILY_ACTION_INPUT) {
      bind (system_automaton_aid, RESPONSE_OUT_NO, 0, aid, desc.number, 0);
    }
  }
  return aid;
}

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    output_bda = buffer_create (0);
    output_bdb = buffer_create (0);
    if (output_bda == -1 || output_bdb == -1) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not create response buffer: %s", lily_error_string (lily_error));
      logs (log_buffer);
      exit (-1);
    }
    if (buffer_file_initw (&output_buffer, output_bda) != 0) {
      snprintf (log_buffer, LOG_BUFFER_SIZE, ERROR "could not initialize response buffer: %s", lily_error_string (lily_error));
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
    de_set (root, "." FS "[0].from.aid", de_create_integer (-1));
    de_set (root, "." FS "[0].from.nodeid", de_create_integer (-1));
    de_set (root, "." FS "[0].to.aid", de_create_integer (tmpfs_aid));
    de_set (root, "." FS "[0].to.nodeid", de_create_integer (0));
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

BEGIN_INPUT (AUTO_PARAMETER, REQUEST_IN_NO, "", "", request_in, ano_t ano, aid_t aid, bd_t bda, bd_t bdb)
{
  initialize ();

  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not initialize request buffer: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  system_msg_type_t type;
  if (system_msg_type_read (&bf, &type) != 0) {
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read request type: %s", lily_error_string (lily_error));
    logs (log_buffer);
    finish_input (bda, bdb);
  }

  switch (type) {
  case CREATE:
    logs (TODO " create");
    break;
  case BIND:
    {
      bind_t b;
      if (bind_read (&bf, &b) != 0) {
	snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "could not read bind request: %s", lily_error_string (lily_error));
	logs (log_buffer);
	finish_input (bda, bdb);
      }
      
      /* Make a record of the bind request. */
      bind_request_t* req = push_bind_request (&b);
      
      /* Send a request for authorization to the three parties. */
      push_bind_rfa (b.output_aid, req, &req->output_approval);
      push_bind_rfa (b.input_aid, req, &req->input_approval);
      push_bind_rfa (b.owner_aid, req, &req->owner_approval);

      bind_fini (&b);
    }
    break;
  case UNBIND:
    logs (TODO " unbind");
    break;
  case DESTROY:
    logs (TODO " destroy");
    break;
  default:
    snprintf (log_buffer, LOG_BUFFER_SIZE, WARNING "unknown request type: %d", type);
    logs (log_buffer);
    finish_input (bda, bdb);
    break;
  }

  finish_input (bda, bdb);
}

BEGIN_OUTPUT (AUTO_PARAMETER, BIND_RFA_OUT_NO, "", "", bind_rfa_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (bind_rfa_head != 0 && bind_rfa_head->to == aid) {
    if (exists (aid) && binding_count (ano, aid) == 1) {
      // Send the rfa.
      buffer_file_truncate (&output_buffer);
      bind_write (&output_buffer, &bind_rfa_head->request->bind);
      pop_bind_rfa ();
      finish_output (true, output_bda, -1);
    }
    else {
      // The automaton in question is either dead or can't receive bind rfa's.
      // We interpret silence as being permissive.
      *bind_rfa_head->approval = GRANTED;
      bind_request_process (bind_rfa_head->request);
      pop_bind_rfa ();
      finish_output (false, -1, -1);
    }
  }

  finish_output (false, -1, -1);
}

BEGIN_OUTPUT (AUTO_PARAMETER, RESPONSE_OUT_NO, "", "", response_out, ano_t ano, aid_t aid)
{
  initialize ();

  if (result_head != 0 && result_head->aid == aid) {
    buffer_file_truncate (&output_buffer);
    
    sysresult_t res;
    sysresult_init (&res, result_head->retval, result_head->error);
    sysresult_write (&output_buffer, &res);
    sysresult_fini (&res);

    pop_result ();
    finish_output (true, output_bda, -1);
  }

  finish_output (false, -1, -1);
}

void
do_schedule (void)
{
  if (bind_rfa_head != 0) {
    schedule (BIND_RFA_OUT_NO, bind_rfa_head->to);
  }
  if (result_head != 0) {
    schedule (RESPONSE_OUT_NO, result_head->aid);
  }
}
