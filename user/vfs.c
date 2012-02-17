#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include <buffer_queue.h>
#include "registry.h"
#include "vfs.h"

/*
  VFS
  ===
  The virtual file system presents a unified hierarchical namespace for managing files.
  
  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REGISTER_REQUEST_NO 1
#define VFS_REGISTER_RESPONSE_NO 2
#define VFS_REQUEST_NO 3
#define VFS_RESPONSE_NO 4

#define DESCRIPTION "vfs"

typedef enum {
  START,
  REGISTER,
  SENT,
  DONE,
} register_state_t;
static register_state_t register_state = START;

/* Initialization flag. */
static bool initialized = false;

/* Queue for responses. */
static buffer_queue_t response_queue;

static void
schedule (void);

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&response_queue);
  }
}

static void
registerf (aid_t vfs_aid)
{
  /* Look up the registry. */
  aid_t registry_aid = get_registry ();
  if (registry_aid == -1) {
    const char* s = "vfs: warning: no registry\n";
    syslog (s, strlen (s));
    return;
  }

  /* Get its description. */
  bd_t bd = describe (registry_aid);
  if (bd == -1) {
    const char* s = "vfs: warning: no registry description\n";
    syslog (s, strlen (s));
    return;
  }

  size_t bd_size = buffer_size (bd);

  /* Map the description. */
  void* ptr = buffer_map (bd);
  if (ptr == 0) {
    const char* s = "vfs: warning: could not map registry description\n";
    syslog (s, strlen (s));
    return;
  }

  const ano_t register_request = action_name_to_number (bd, bd_size, ptr, REGISTRY_REGISTER_REQUEST_NAME);
  const ano_t register_response = action_name_to_number (bd, bd_size, ptr, REGISTRY_REGISTER_RESPONSE_NAME);

  /* Dispense with the buffer. */
  buffer_destroy (bd);

  if (bind (vfs_aid, VFS_REGISTER_REQUEST_NO, 0, registry_aid, register_request, 0) == -1 ||
      bind (registry_aid, register_response, 0, vfs_aid, VFS_REGISTER_RESPONSE_NO, 0) == -1) {
    const char* s = "vfs: warning: couldn't bind to registry\n";
    syslog (s, strlen (s));
  }
  else {
    register_state = REGISTER;
  }

}

static void
form_response (aid_t aid,
	       vfs_type_t type,
	       vfs_error_t error)
{
  /* Create a response. */
  bd_t bd = buffer_create (size_to_pages (sizeof (vfs_response_t)));
  vfs_response_t* rr = buffer_map (bd);
  rr->type = type;
  rr->error = error;
  /* We don't unmap it because we are going to destroy it when we respond. */

  buffer_queue_push (&response_queue, aid, bd);
}

static void
process_request (aid_t aid,
		 bd_t bd,
		 size_t bd_size)
{
  if (bd == -1) {
    form_response (aid, VFS_UNKNOWN, VFS_NO_BUFFER);
    return;
  }

  /* Create a buffer that contains the request. */
  bd_t request_bd = buffer_copy (bd, 0, size_to_pages (sizeof (vfs_request_t)));
  if (request_bd == -1) {
    form_response (aid, VFS_UNKNOWN, VFS_NO_MAP);
    return;
  }

  /* Map in the request. */
  void* ptr= buffer_map (request_bd);
  if (ptr == 0) {
    form_response (aid, VFS_UNKNOWN, VFS_NO_MAP);
    return;
  }

  buffer_file_t file;
  buffer_file_open (&file, bd, bd_size, ptr, false);

  const vfs_request_t* r = buffer_file_readp (&file, sizeof (vfs_request_t));
  if (r == 0) {
    form_response (aid, VFS_UNKNOWN, VFS_BAD_REQUEST);
    return;
  }

  switch (r->type) {
  case VFS_MOUNT:
    {
      if (r->u.mount.path_size == 0) {
	/* No path. */
	form_response (aid, VFS_MOUNT, VFS_BAD_PATH);
	return;
      }

      const char* path = buffer_file_readp (&file, r->u.mount.path_size);
      if (path == 0) {
	/* Path is too long or unreadable. */
	form_response (aid, VFS_MOUNT, VFS_BAD_PATH);
	return;
      }

      if (path[0] != '/') {
	/* Must be absolute. */
	form_response (aid, VFS_MOUNT, VFS_BAD_PATH);
	return;
      }

      if (path[r->u.mount.path_size - 1] != 0) {
	/* Path is not null terminated. */
	form_response (aid, VFS_MOUNT, VFS_BAD_PATH);
	return;
      }

      /* TODO */
      syslog (path, r->u.mount.path_size);

      return;
    }
    break;
  default:
    form_response (aid, VFS_UNKNOWN, VFS_BAD_REQUEST);
    return;
  }
}

void
init (aid_t vfs_aid,
      bd_t bd,
      size_t bd_size,
      void* ptr)
{
  initialize ();
  registerf (vfs_aid);
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, 0, init, INIT, "", "");

static bool
register_request_precondition (void)
{
  return register_state == REGISTER;
}

void
register_request (int param,
		  size_t bc)
{
  initialize ();
  scheduler_remove (VFS_REGISTER_REQUEST_NO, param);

  if (register_request_precondition ()) {
    buffer_file_t file;
    buffer_file_create (&file, sizeof (registry_register_request_t));
    registry_register_request_t r;
    r.method = REGISTRY_STRING_EQUAL;
    r.description_size = strlen (DESCRIPTION) + 1;
    buffer_file_write (&file, &r, sizeof (registry_register_request_t));
    buffer_file_write (&file, DESCRIPTION, r.description_size);
    bd_t bd = buffer_file_bd (&file);

    register_state = SENT;

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NOOP);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, 0, register_request, VFS_REGISTER_REQUEST_NO, "", "");

void
register_response (int param,
		   bd_t bd,
		   size_t bd_size,
		   void* ptr)
{
  initialize ();

  if (ptr != 0) {
    buffer_file_t file;
    buffer_file_open (&file, bd, bd_size, ptr, false);
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
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, AUTO_MAP, register_response, VFS_REGISTER_RESPONSE_NO, "", "");

void
request (aid_t aid,
	 bd_t bd,
	 size_t bd_size,
	 void* ptr)
{
  initialize ();

  if (bd == -1) {
    /* TODO */
    const char* s = "vfs: no buffer\n";
    syslog (s, strlen (s));
  }

  process_request (aid, bd, bd_size);

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, 0, request, VFS_REQUEST_NO, VFS_REQUEST_NAME, "");

void
response (aid_t aid,
	  size_t bc)
{
  initialize ();
  scheduler_remove (VFS_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&response_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&response_queue, item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (-1, FINISH_NOOP);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, AUTO_PARAMETER, 0, response, VFS_RESPONSE_NO, VFS_RESPONSE_NAME, "");

static void
schedule (void)
{
  if (register_request_precondition ()) {
    scheduler_add (VFS_REGISTER_REQUEST_NO, 0);
  }
  if (!buffer_queue_empty (&response_queue)) {
    scheduler_add (VFS_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&response_queue)));
  }
  /* TODO */
}
