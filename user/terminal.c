#include "terminal.h"
#include "vga.h"
#include <automaton.h>
#include <string.h>
#include <dymem.h>
#include <fifo_scheduler.h>
#include <buffer_heap.h>

/*
  VGA Terminal Server
  ===================
  
  This server implements a terminal on a video graphics array (VGA).
  The server interprets a stream of a data and control sequences and renders them to a VGA.
  The server is designed for multiple clients where each client manipulates a private virtual terminal.
  The server provides a mechanism for choosing which virtual terminal is displayed on the VGA.

  Interface
  ---------
  FOCUS - Determine which client is rendered to the VGA.
  DISPLAY - Receive data and control sequences to be rendered.
  VGA_OP - Send operations to control the VGA.

  The ECMA-48 Standard
  --------------------
  The goal is that the terminal server will conform to the ECMA-48 Standard.
  ECMA-48 specifies how control functions are encoded and interpretted.
  Conforming to this standard does not mean implementing every control function.
  Rather confomances means:
  (1) a control sequences specified in the standard is always interpretted as a control sequences;
  (2) a function specified in the standard must always be code by the representation in the standard;
  (3) reserved functions are ignored; and
  (4) functions that are supported must be identified.
  See Section 2 of the ECMA-48 Standard for details.

  Status
  ------
  Data must be encoded using 7-bit ASCII.
  The following control codes are supported:
    
  Limitations
  -----------
  The server assumes an 80x25 single page display.
  The character progression is assumed left-to-right and the line progression is assumed top-to-bottom.

  Authors
  -------
  Justin R. Wilson

*/

#define LINE_HOME_POSITION 0
#define LINE_LIMIT_POSITION 80
#define PAGE_HOME_POSITION 0
#define PAGE_LIMIT_POSITION 25

/*
  Size of a character cell.
  1 byte contains the character.
  1 byte contains attributes.
*/
#define CELL_SIZE 2

/* White on black. */
#define ATTRIBUTE 0x0F00

typedef struct client client_t;
struct client {
  /* Associated aid. */
  aid_t aid;
  /* Buffer descriptor for the data associated with this client. */
  bd_t bd;
  /* Same only mapped in. */
  unsigned short* buffer;
  /* Active position.  These are zero based. */
  unsigned short active_position_x;
  unsigned short active_position_y;
  /* Next on the list. */
  client_t* next;
};

/* The list of clients. */
static client_t* client_list_head = 0;
/* The active client. */
static client_t* active_client = 0;
/* Flag indicating that the data for the active_client needs to be sent to the VGA. */
static bool refresh = false;
/* The number of actions bound to the terminal_vga_op output.  By making this non-zero to start, we force an update. */
static size_t terminal_vga_op_binding_count = 1;

static client_t*
find_client (aid_t aid)
{
  client_t* ptr;
  for (ptr = client_list_head; ptr != 0 && ptr->aid != aid; ptr = ptr->next) ;;
  return ptr;
}

static client_t*
create_client (aid_t aid)
{
  client_t* client = malloc (sizeof (client_t));
  client->aid = aid;
  client->bd = buffer_create (PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE);
  client->buffer = buffer_map (client->bd);
  /* ECMA-48 states that the initial state of the characters must be "erased."
     The buffer should be all zeroes but just in case things change...*/
  memset (client->buffer, 0, PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE);
  /* I don't recall ECMA-48 specifying the initial state of the active position but this seems reasonable. */
  client->active_position_x = LINE_HOME_POSITION;
  client->active_position_y = PAGE_HOME_POSITION;

  client->next = client_list_head;
  client_list_head = client;

  subscribe_destroyed (aid);

  return client;
}

static void
switch_to_client (client_t* client)
{
  if (client != active_client) {
    active_client = client;
    refresh = true;
  }
}

static void
destroy_client (client_t* client)
{
  if (client == active_client) {
    /* Switch to the null client. */
    switch_to_client (0);
  }
  buffer_destroy (client->bd);
  free (client);
}

static void
scroll (client_t* client)
{
  /* Move the data up one line. */
  memmove (client->buffer, &client->buffer[LINE_LIMIT_POSITION], (PAGE_LIMIT_POSITION - 1) * LINE_LIMIT_POSITION * CELL_SIZE);
  /* Clear the last line. */
  memset (&client->buffer[(PAGE_LIMIT_POSITION - 1) * LINE_LIMIT_POSITION], 0, LINE_LIMIT_POSITION * CELL_SIZE);
  /* Change the active y. */
  --client->active_position_y;
}

static void schedule (void);

typedef struct {
  aid_t aid;
} focus_arg_t;

void
terminal_focus (int param,
		bd_t bd,
		size_t buffer_size,
		focus_arg_t* a)
{
  if (a != 0 && buffer_size >= sizeof (focus_arg_t)) {
    /* We don't care if the client is null. */
    switch_to_client (find_client (a->aid));
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, TERMINAL_FOCUS, terminal_focus);

void
terminal_display (aid_t aid,
		  bd_t bd,
		  size_t buffer_size,
		  void* ptr)
{
  if (ptr == 0) {
    /* Either no data or we can't map it. */
    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }

  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, buffer_size);
  
  const terminal_display_arg_t* arg = buffer_heap_alloc (&heap, sizeof (terminal_display_arg_t));
  if (arg == 0) {
    /* Not enough data. */
    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }

  const char* string = (void*)&arg->string + arg->string;
  if (!buffer_heap_check (&heap, string, arg->size)) {
    /* Not enough data. */
    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }




  /* Find or create the client. */
  client_t* client = find_client (aid);
  if (client == 0) {
    client = create_client (aid);
  }
  
  /* Process the string. */
  const char* end = begin + buffer_size;
  for (; begin != end; ++begin) {
    const char c = *begin;
    if (c >= 0 && c < 0x20) {
      /* C0 control character. */
      /* TODO */
    }
    else if (c >= 0x20 && c < 0x7F) {
      /* ASCII character that can be displayed. */
      
      if (client->active_position_x == LINE_LIMIT_POSITION) {
	/* Active position is at the end of the line.
	   ECMA-48 does not define behavior in this circumstance.
	   We will move to home and scroll.
	*/
	client->active_position_x = LINE_HOME_POSITION;
	++client->active_position_y;
      }
      
      if (client->active_position_y == PAGE_LIMIT_POSITION) {
	/* Same idea. */
	scroll (client);
      }
      
      client->buffer[client->active_position_y * LINE_LIMIT_POSITION + client->active_position_x++] = ATTRIBUTE | c;
    }
    else if (c == 0x7F) {
      /* ASCII DEL code. */
      /* TODO */
    }
  }
  
  /* If this is the active client, refresh the VGA. */
  if (client == active_client) {
    refresh = true;
  }
  
  /* TODO:  Remove this line. */
  switch_to_client (client);

  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, TERMINAL_DISPLAY, terminal_display);

static bool
terminal_vga_precondition (void)
{
  return refresh && terminal_vga_op_binding_count != 0;
}

void
terminal_vga_op (int param,
		size_t bc)
{
  scheduler_remove (TERMINAL_VGA_OP, param);
  terminal_vga_op_binding_count = bc;

  if (terminal_vga_precondition ()) {
    if (active_client != 0) {
      /* Send the data. */
      size_t buffer_size = sizeof (vga_op_t);
      bd_t bd = buffer_create (buffer_size);
      size_t data_offset = buffer_append (bd, client->bd);
      vga_op_t* op = buffer_map (bd);
      op->type = VGA_ASSIGN;
      op->arg.assign.address = 0;
      op->arg.assign.size = PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE;
      

      

      /* TODO */
      /* Send the cursor location. */
    }
    else {
      /* TODO */
    }

    /* vga_op_item_t* item = vga_op_queue_pop (); */
    /* bd_t bd = item->bd; */
    /* size_t buffer_size = item->buffer_size; */
    /* free (item); */

    bd_t bd = -1;
    size_t buffer_size = 0;

    schedule ();
    scheduler_finish (bd, buffer_size, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, 0, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, TERMINAL_VGA_OP, terminal_vga_op);

void
destroyed (aid_t aid,
	   bd_t bd,
	   size_t buffer_size,
	   void* p)
{
  /* Destroy the client. */
  client_t** ptr = &client_list_head;
  for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;;
  if (*ptr != 0) {
    client_t* temp = *ptr;
    *ptr = temp->next;
    destroy_client (temp);
  }

  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, DESTROYED, destroyed);

static void
schedule (void)
{
  if (terminal_vga_precondition ()) {
    scheduler_add (TERMINAL_VGA_OP, 0);
  }
}
