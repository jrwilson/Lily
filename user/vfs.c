#include <automaton.h>
#include <io.h>
#include <string.h>
#include <fifo_scheduler.h>
#include <buffer_file.h>
#include <buffer_queue.h>
#include <description.h>
#include "registry.h"
#include "vfs.h"

/*
  VFS
  ===
  The virtual file system presents a unified hierarchical namespace for managing files.
  The VFS translates high-level requests from clients into low-level file-system operations.
  
  Client 1              File system A
  Client 2  <-> VFS <-> File system B
  Client 3              File system C
  ...                   ...
  
  Implementation
  --------------
  The implementation is based around four queues called the client request queue, the client response queue, the file system request queue, and the file system response queue.
  The core algorithm takes items from the client request queue and file system response queue and puts items on the client response queue and file system request queue.
  client requests            file system requests
                   Processor
  client responses           file system responses

  Assume that a client request Q has a direct mapping to the file system request Q'.
  Suppose also that the responses are R and R', respectively.
  If no other requests are made, the operation of the system resembles:

  Start empty:
  (empty)           (empty)             
          Processor
  (empty)           (empty)              

  Client makes a request:
  Q                 (empty)             
          Processor
  (empty)           (empty)              

  Request is translated to file system operation (AA):
  Q                 Q'             
          Processor
  (empty)           (empty)              

  File system request sent (BB):
  Q                 (empty)
          Processor
  (empty)           (empty)              

  File system response received (CC):
  Q                 (empty)
          Processor
  (empty)           R'

  A client response is generated and the request is consumed:
  (empty)           (empty)
          Processor
  R                 (empty)

  The client request is sent:
  (empty)           (empty)
          Processor
  (empty)           (empty)

  Obviously, there is not always a one-to-one mapping between client requests and file system requests.
  In this case, the sequence AA-BB-CC can be repeated as many times as are necessary to fulfill the request.

  This implementation is not very efficient as it serializes everything.
  Performance can be increased by exploiting situations where client requests are independent and the fact that file system are independent.
  Thus, instead of having one pair of queues for client requests, one could have a queue for each client.
  Similarly, instead of having one pair of queues for file system requests, one could have a queue for each file system.
    
  Authors:  Justin R. Wilson
  Copyright (C) 2012 Justin R. Wilson
*/

#define VFS_REGISTER_REQUEST_NO 1
#define VFS_REGISTER_RESPONSE_NO 2
#define VFS_REQUEST_NO 3
#define VFS_RESPONSE_NO 4
#define VFS_FS_REQUEST_NO 5
#define VFS_FS_RESPONSE_NO 6
#define DESTROY_BUFFERS_NO 7

#define DESCRIPTION "vfs"

/* Initialization flag. */
static bool initialized = false;

/* State machine for registration. */
typedef enum {
  START,
  REGISTER,
  SENT,
  DONE,
} register_state_t;
static register_state_t register_state = START;

/* Queue for client requests. */
static buffer_queue_t client_request_queue;

/* Queue for file system requests. */
static buffer_queue_t file_system_request_queue;

/* Queue for file system responses. */
static buffer_queue_t file_system_response_queue;

/* Queue for client responses. */
static buffer_queue_t client_response_queue;

/* Queue of buffers to destroy. */
static buffer_queue_t destroy_queue;

static void
schedule (void);

static void
initialize (void)
{
  if (!initialized) {
    initialized = true;

    buffer_queue_init (&client_request_queue);
    buffer_queue_init (&file_system_request_queue);
    buffer_queue_init (&file_system_response_queue);
    buffer_queue_init (&client_response_queue);
    buffer_queue_init (&destroy_queue);
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
    buffer_destroy (bd);
    return;
  }

  const ano_t register_request = action_name_to_number (bd, bd_size, ptr, REGISTRY_REGISTER_REQUEST_NAME);
  const ano_t register_response = action_name_to_number (bd, bd_size, ptr, REGISTRY_REGISTER_RESPONSE_NAME);

  /* Dispense with the buffer. */
  buffer_destroy (bd);

  if (bind (registry_aid, register_response, 0, vfs_aid, VFS_REGISTER_RESPONSE_NO, 0) == -1 ||
      bind (vfs_aid, VFS_REGISTER_REQUEST_NO, 0, registry_aid, register_request, 0) == -1) {
    const char* s = "vfs: warning: couldn't bind to registry\n";
    syslog (s, strlen (s));
  }
  else {
    register_state = REGISTER;
  }
}

/* static void */
/* form_response (aid_t aid, */
/* 	       vfs_type_t type, */
/* 	       vfs_error_t error) */
/* { */
/*   /\* Create a response. *\/ */
/*   bd_t bd = buffer_create (size_to_pages (sizeof (vfs_response_t))); */
/*   vfs_response_t* rr = buffer_map (bd); */
/*   rr->type = type; */
/*   rr->error = error; */
/*   /\* We don't unmap it because we are going to destroy it when we respond. *\/ */

/*   buffer_queue_push (&response_queue, aid, bd); */
/* } */

/* static void */
/* process_request (aid_t aid, */
/* 		 bd_t bd, */
/* 		 size_t bd_size) */
/* { */
/*   if (bd == -1) { */
/*     form_response (aid, VFS_UNKNOWN, VFS_NO_BUFFER); */
/*     return; */
/*   } */

/*   /\* Create a buffer that contains the request. *\/ */
/*   bd_t request_bd = buffer_copy (bd, 0, size_to_pages (sizeof (vfs_request_t))); */
/*   if (request_bd == -1) { */
/*     form_response (aid, VFS_UNKNOWN, VFS_NO_MAP); */
/*     return; */
/*   } */

/*   /\* Map in the request. *\/ */
/*   void* ptr= buffer_map (request_bd); */
/*   if (ptr == 0) { */
/*     form_response (aid, VFS_UNKNOWN, VFS_NO_MAP); */
/*     return; */
/*   } */

/*   buffer_file_t file; */
/*   buffer_file_open (&file, bd, bd_size, ptr, false); */

/*   const vfs_request_t* r = buffer_file_readp (&file, sizeof (vfs_request_t)); */
/*   if (r == 0) { */
/*     form_response (aid, VFS_UNKNOWN, VFS_BAD_REQUEST); */
/*     return; */
/*   } */

/*   switch (r->type) { */
/*   case VFS_MOUNT: */
/*     { */
/*       if (r->u.mount.path_size == 0) { */
/* 	/\* No path. *\/ */
/* 	form_response (aid, VFS_MOUNT, VFS_BAD_PATH); */
/* 	return; */
/*       } */

/*       const char* path = buffer_file_readp (&file, r->u.mount.path_size); */
/*       if (path == 0) { */
/* 	/\* Path is too long or unreadable. *\/ */
/* 	form_response (aid, VFS_MOUNT, VFS_BAD_PATH); */
/* 	return; */
/*       } */

/*       if (path[0] != '/') { */
/* 	/\* Must be absolute. *\/ */
/* 	form_response (aid, VFS_MOUNT, VFS_BAD_PATH); */
/* 	return; */
/*       } */

/*       if (path[r->u.mount.path_size - 1] != 0) { */
/* 	/\* Path is not null terminated. *\/ */
/* 	form_response (aid, VFS_MOUNT, VFS_BAD_PATH); */
/* 	return; */
/*       } */

/*       /\* TODO *\/ */
/*       syslog (path, r->u.mount.path_size); */

/*       return; */
/*     } */
/*     break; */
/*   default: */
/*     form_response (aid, VFS_UNKNOWN, VFS_BAD_REQUEST); */
/*     return; */
/*   } */
/* } */

BEGIN_SYSTEM_INPUT (INIT, "", "", init, aid_t vfs_aid, bd_t bd, size_t bd_size)
{
  {
    const char* s = "vfs: init\n";
    syslog (s, strlen (s));
  }

  initialize ();
  registerf (vfs_aid);
  buffer_destroy (bd);
  schedule ();
  scheduler_finish (false, -1);
}

static bool
register_request_precondition (void)
{
  return register_state == REGISTER;
}

BEGIN_OUTPUT (NO_PARAMETER, VFS_REGISTER_REQUEST_NO, "", "", reqister_request, int param, size_t bc)
{
  {
    const char* s = "vfs: register_request\n";
    syslog (s, strlen (s));
  }

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

    buffer_queue_push (&destroy_queue, 0, bd);

    schedule ();
    scheduler_finish (true, bd);
  }
  else {
    schedule ();
    scheduler_finish (false, -1);
  }
}

BEGIN_INPUT (NO_PARAMETER, VFS_REGISTER_RESPONSE_NO, "", "", register_response, int param, bd_t bd, size_t bd_size)
{
  {
    const char* s = "vfs: register_response\n";
    syslog (s, strlen (s));
  }

  initialize ();

  void* ptr = buffer_map (bd);

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
  else {
    const char* s = "vfs: warning: couldn't map registry response\n";
    syslog (s, strlen (s));
  }

  schedule ();
  scheduler_finish (false, -1);
}

BEGIN_INPUT (AUTO_PARAMETER, VFS_REQUEST_NO, VFS_REQUEST_NAME, "", client_request, aid_t aid, bd_t bd, size_t bd_size)
{
  {
    const char* s = "vfs: client_request\n";
    syslog (s, strlen (s));
  }

  initialize ();
  buffer_queue_push (&client_request_queue, aid, bd);
  schedule ();
  scheduler_finish (false, -1);
}

BEGIN_OUTPUT (AUTO_PARAMETER, VFS_RESPONSE_NO, VFS_RESPONSE_NAME, "", client_response, aid_t aid, size_t bc)
{
  {
    const char* s = "vfs: client_response\n";
    syslog (s, strlen (s));
  }

  initialize ();
  scheduler_remove (VFS_RESPONSE_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&client_response_queue, aid);

  if (item != 0) {
    /* Found a response.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&client_response_queue, item);

    buffer_queue_push (&destroy_queue, 0, bd);

    schedule ();
    scheduler_finish (true, bd);
  }
  else {
    /* Did not find a response. */
    schedule ();
    scheduler_finish (false, -1);
  }
}

BEGIN_OUTPUT (AUTO_PARAMETER, VFS_FS_REQUEST_NO, "", "", file_system_request, aid_t aid, size_t bc)
{
  {
    const char* s = "vfs: file_system_request\n";
    syslog (s, strlen (s));
  }

  initialize ();
  scheduler_remove (VFS_FS_REQUEST_NO, aid);

  /* Find in the queue. */
  buffer_queue_item_t* item = buffer_queue_find (&file_system_request_queue, aid);

  if (item != 0) {
    /* Found a request.  Execute. */
    bd_t bd = buffer_queue_item_bd (item);
    buffer_queue_erase (&file_system_request_queue, item);

    buffer_queue_push (&destroy_queue, 0, bd);

    schedule ();
    scheduler_finish (true, bd);
  }
  else {
    /* Did not find a request. */
    schedule ();
    scheduler_finish (false, -1);
  }
}

BEGIN_INPUT (AUTO_PARAMETER, VFS_FS_RESPONSE_NO, "", "", file_system_response, aid_t aid, bd_t bd, size_t bd_size)
{
  {
    const char* s = "vfs: file_system_response\n";
    syslog (s, strlen (s));
  }

  initialize ();
  buffer_queue_push (&file_system_response_queue, aid, bd);
  schedule ();
  scheduler_finish (false, -1);
}

static bool
destroy_buffers_precondition (void)
{
  return !buffer_queue_empty (&destroy_queue);
}

BEGIN_INTERNAL (NO_PARAMETER, DESTROY_BUFFERS_NO, "", "", destroy_buffers, int param)
{
  {
    const char* s = "boot_automaton: destroy_buffers\n";
    syslog (s, strlen (s));
  }

  initialize ();
  scheduler_remove (DESTROY_BUFFERS_NO, param);
  if (destroy_buffers_precondition ()) {
    /* Drain the queue. */
    while (!buffer_queue_empty (&destroy_queue)) {
      buffer_destroy (buffer_queue_item_bd (buffer_queue_front (&destroy_queue)));
      buffer_queue_pop (&destroy_queue);
    }
  }
  schedule ();
  scheduler_finish (false, -1);
}

static void
schedule (void)
{
  if (register_request_precondition ()) {
    scheduler_add (VFS_REGISTER_REQUEST_NO, 0);
  }
  if (!buffer_queue_empty (&client_response_queue)) {
    scheduler_add (VFS_RESPONSE_NO, buffer_queue_item_parameter (buffer_queue_front (&client_response_queue)));
  }
  if (!buffer_queue_empty (&file_system_request_queue)) {
    scheduler_add (VFS_FS_REQUEST_NO, buffer_queue_item_parameter (buffer_queue_front (&file_system_request_queue)));
  }
  if (destroy_buffers_precondition ()) {
    scheduler_add (DESTROY_BUFFERS_NO, 0);
  }
}
