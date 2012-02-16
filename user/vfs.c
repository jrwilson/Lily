#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include "registry.h"

ano_t
action_name_to_number (bd_t bd,
		       size_t bd_size,
		       void* ptr,
		       const char* action_name)
{
  buffer_file_t bf;
  buffer_file_open (&bf, bd, bd_size, ptr, false);
  const size_t* count = buffer_file_readp (&bf, sizeof (size_t));
  if (count == 0) {
    return NO_ACTION;
  }

  for (size_t idx = 0; idx != *count; ++idx) {
    const action_descriptor_t* d = buffer_file_readp (&bf, sizeof (action_descriptor_t));
    if (d == 0) {
      return NO_ACTION;
    }

    const char* name = buffer_file_readp (&bf, d->name_size);
    if (name == 0) {
      return NO_ACTION;
    }

    const char* description = buffer_file_readp (&bf, d->description_size);
    if (description == 0) {
      return NO_ACTION;
    }

    if (strcmp (name, action_name) == 0) {
      return d->number;
    }
  }

  return NO_ACTION;
}

#define VFS_REGISTER_REQUEST 1
#define VFS_REGISTER_RESPONSE 2

static const char* description = "vfs";

typedef enum {
  START,
  REGISTER,
  SENT,
  DONE,
} register_state_t;
static register_state_t register_state = START;

static void
schedule (void);

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
    const char* s = "vfs: warning: could map registry description\n";
    syslog (s, strlen (s));
    return;
  }

  const ano_t register_request = action_name_to_number (bd, bd_size, ptr, REGISTRY_REGISTER_REQUEST_NAME);
  const ano_t register_response = action_name_to_number (bd, bd_size, ptr, REGISTRY_REGISTER_RESPONSE_NAME);

  /* Dispense with the buffer. */
  buffer_destroy (bd);

  if (bind (vfs_aid, VFS_REGISTER_REQUEST, 0, registry_aid, register_request, 0) == -1 ||
      bind (registry_aid, register_response, 0, vfs_aid, VFS_REGISTER_RESPONSE, 0) == -1) {
    const char* s = "vfs: warning: couldn't bind to registry\n";
    syslog (s, strlen (s));
  }
  else {
    register_state = REGISTER;
  }

}

void
init (aid_t vfs_aid,
      bd_t bd,
      size_t bd_size,
      void* ptr)
{
  registerf (vfs_aid);
  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, 0, init, INIT, "init", "description");

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
    r.description_size = strlen (description) + 1;
    buffer_file_write (&file, &r, sizeof (registry_register_request_t));
    buffer_file_write (&file, description, r.description_size);
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
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, 0, register_request, VFS_REGISTER_REQUEST, "register_request", "description");

void
register_response (int param,
		   bd_t bd,
		   size_t bd_size,
		   void* ptr)
{
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
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, AUTO_MAP, register_response, VFS_REGISTER_RESPONSE, "register_response", "description");

static void
schedule (void)
{
  if (register_request_precondition ()) {
    scheduler_add (VFS_REGISTER_REQUEST, 0);
  }
  /* TODO */
}
