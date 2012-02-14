#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include "registry.h"

#define VFS_REGISTER_REQUEST 1
#define VFS_REGISTER_RESPONSE 2

#define VFS_FILE_REQUEST 3
typedef enum {
  VFS_READ_FILE,
} vfs_file_request_type_t;

typedef struct {
  vfs_file_request_type_t type;
  union {
    struct {
      size_t path_size;	/* Size of the path that follows. */
    } read_file;
  } u;
} vfs_file_request_t;

#define VFS_FILE_RESPONSE 4

static const char* description = "vfs";

/* Initialization flag. */
static bool initialized = false;

typedef enum {
  START,
  REGISTER,
  SENT,
  DONE,
} register_state_t;
static register_state_t register_state = START;

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;
  }
}

static void
schedule (void);

void
init (aid_t vfs_aid,
      bd_t bd,
      const void* ptr,
      size_t capacity)
{
  initialize ();

  /* Look up the registry and bind to it. */
  aid_t registry_aid = get_registry ();
  if (registry_aid != -1) {
    if (bind (vfs_aid, VFS_REGISTER_REQUEST, 0, registry_aid, REGISTRY_REGISTER_REQUEST, 0) == -1 ||
	bind (registry_aid, REGISTRY_REGISTER_RESPONSE, 0, vfs_aid, VFS_REGISTER_RESPONSE, 0) == -1) {
      const char* s = "vfs: warning: couldn't bind to registry\n";
      syslog (s, strlen (s));
    }
    else {
      register_state = REGISTER;
    }
  }
  else {
    const char* s = "vfs: warning: no registry\n";
    syslog (s, strlen (s));
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, 0, INIT, init);

static bool
register_request_precondition (void)
{
  return register_state == REGISTER;
}

void
register_request (int param,
		  size_t bc)
{
  if (register_request_precondition ()) {
    buffer_file_t file;
    buffer_file_create (&file, sizeof (registry_register_request_t));
    registry_register_request_t r;
    r.method = REGISTRY_STRING_EQUAL;
    r.description_size = strlen (description);
    buffer_file_write (&file, &r, sizeof (registry_register_request_t));
    buffer_file_write (&file, description, r.description_size);
    bd_t bd = buffer_file_bd (&file);

    register_state = SENT;

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, 0, VFS_REGISTER_REQUEST, register_request);

void
register_response (int param,
		   bd_t bd,
		   void* ptr,
		   size_t capacity)
{
  if (ptr != 0) {
    buffer_file_t file;
    buffer_file_open (&file, false, bd, ptr, capacity);
    const registry_register_response_t* r = buffer_file_readp (&file, sizeof (registry_register_response_t));
    if (r != 0) {
      switch (r->error) {
      case REGISTRY_SUCCESS:
	/* Okay. */
	break;
      default:
	{
	  const char* s = "vfs: warning: failed to register\n";
	  syslog (s, strlen (s));
	}
	break;
      }
    }
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, AUTO_MAP, VFS_REGISTER_RESPONSE, register_response);

static void
process_file_request (aid_t aid,
		      bd_t bd,
		      size_t capacity)
{
  if (bd == -1) {
    /* TODO */
    return;
  }

  /* TODO */
}

void
file_request (aid_t aid,
	      bd_t bd,
	      void* ptr,
	      size_t capacity)
{
  process_file_request (aid, bd, capacity);

  /* TODO */

  /* if (ptr != 0) { */
  /*   buffer_file_t file; */
  /*   buffer_file_open (&file, false, bd, ptr, capacity); */
  /*   const registry_file_request_t* r = buffer_file_readp (&file, sizeof (registry_file_request_t)); */
  /*   if (r != 0) { */
  /*     switch (r->error) { */
  /*     case REGISTRY_SUCCESS: */
  /* 	/\* Okay. *\/ */
  /* 	break; */
  /*     default: */
  /* 	{ */
  /* 	  const char* s = "vfs: warning: failed to register\n"; */
  /* 	  syslog (s, strlen (s)); */
  /* 	} */
  /* 	break; */
  /*     } */
  /*   } */
  /* } */


  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, 0, VFS_FILE_REQUEST, file_request);

static bool
file_response_precondition (void)
{
  return register_state == REGISTER;
}

void
file_response (int param,
		  size_t bc)
{
  /* TODO */
  /* if (file_response_precondition ()) { */
  /*   buffer_file_t file; */
  /*   buffer_file_create (&file, sizeof (registry_file_response_t)); */
  /*   registry_file_response_t r; */
  /*   r.method = REGISTRY_STRING_EQUAL; */
  /*   r.description_size = strlen (description); */
  /*   buffer_file_write (&file, &r, sizeof (registry_file_response_t)); */
  /*   buffer_file_write (&file, description, r.description_size); */
  /*   bd_t bd = buffer_file_bd (&file); */

  /*   register_state = SENT; */

  /*   schedule ();p */
  /*   scheduler_finish (bd, FINISH_DESTROY); */
  /* } */
  /* else { */
  /*   schedule (); */
  /*   scheduler_finish (-1, FINISH_NO); */
  /* } */

  schedule ();
  scheduler_finish (-1, FINISH_NO);
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, 0, VFS_FILE_RESPONSE, file_response);

static void
schedule (void)
{
  if (register_request_precondition ()) {
    scheduler_add (VFS_REGISTER_REQUEST, 0);
  }
  if (file_response_precondition ()) {
    /* TODO: parameter */
    scheduler_add (VFS_FILE_RESPONSE, 0);
  }
  /* TODO */
}
