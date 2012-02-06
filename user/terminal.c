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

/* Null. */
#define NUL 0X00
/* Start of Heading (ISO 1745). */
#define SOH 0X01
/* Start of Text (ISO 1745). */
#define STX 0X02
/* End of Text (ISO 1745). */
#define ETX 0X03
/* End of Transmission (ISO 1745). */
#define EOT 0X04
/* Enquiry  (ISO 1745). */
#define ENQ 0X05
/* Acknowledge  (ISO 1745). */
#define ACK 0X06
/* Bell. */
#define BEL 0X07
/* Backspace. */
#define BS  0X08
/* Character Tabulation. */
#define HT  0X09
/* Line Feed. */
#define LF  0x0A
/* Line Tabulation. */
#define VT  0x0B
/* Form Feed. */
#define FF  0x0C
/* Carriage Return. */
#define CR  0x0D
/* Shift-Out (ECMA-35). */
#define SO  0x0E
/* Shift-In (ECMA-35). */
#define SI  0x0F
/* Data Link Escape (ISO 1745). */
#define DLE 0x10
/* Device Control One (XON). */
#define DC1 0x11
/* Device Control Two. */
#define DC2 0x12
/* Device Control Three (XOFF). */
#define DC3 0x13
/* Device Control Four. */
#define DC4 0x14
/* Negative Acknowledge (ISO 1745). */
#define NAK 0x15
/* Synchronous Idle (ISO 1745). */
#define SYN 0x16
/* End of Transmission Block (ISO 1745). */
#define ETB 0x17
/* Cancel. */
#define CAN 0x18
/* End of Medium. */
#define EM  0x19
/* Substitute. */
#define SUB 0x1A
/* Escape (ECMA-35). */
#define ESC 0x1B
/* Information Separator Four (FS - File Separator). */
#define IS4 0x1C
/* Information Separator Three (GS - Group Separator). */
#define IS3 0x1D
/* Information Separator Two (RS - Record Separator). */
#define IS2 0x1E
/* Information Separator One (US - Unit Separator). */
#define IS1 0x1F

/* The ASCII delete character. */
#define DEL 0x7F

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
  unsigned short* data = client->buffer;
  for (size_t y = 0; y != PAGE_LIMIT_POSITION; ++y) {
    for (size_t x = 0; x != LINE_LIMIT_POSITION; ++x) {
      data[y * LINE_LIMIT_POSITION + x] = ATTRIBUTE | ' ';
    }
  }
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
    /* Only refresh if there is a client. */
    refresh = active_client != 0;
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
		focus_arg_t* a,
		size_t buffer_size)
{
  if (a != 0 && buffer_size >= sizeof (focus_arg_t)) {
    /* We don't care if the client is null. */
    switch_to_client (find_client (a->aid));
  }

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, TERMINAL_FOCUS, terminal_focus);

static void
display (aid_t aid,
	 void* ptr,
	 size_t buffer_size)
{
  if (ptr == 0) {
    /* Either no data or we can't map it. */
    return;
  }
  
  buffer_heap_t heap;
  buffer_heap_init (&heap, ptr, buffer_size);
  
  const terminal_display_arg_t* arg = buffer_heap_begin (&heap);
  if (!buffer_heap_check (&heap, arg, sizeof (terminal_display_arg_t))) {
    /* Not enough data. */
    return;
  }

  const char* begin = (void*)&arg->string + arg->string;
  if (!buffer_heap_check (&heap, begin, arg->size)) {
    /* Not enough data. */
    return;
  }

  /* Find or create the client. */
  client_t* client = find_client (aid);
  if (client == 0) {
    client = create_client (aid);
  }
  
  /* Process the string. */
  const char* end = begin + arg->size;
  for (; begin != end; ++begin) {
    const char c = *begin;

    if (c >= NUL && c <= DEL) {
      /* Process a 7-bit ASCII character. */
      switch (c) {
      case NUL:
      case SOH:
      case STX:
      case ETX:
      case EOT:
      case ENQ:
      case ACK:
	/* Do nothing. */
	break;
      case BEL:
	/* Not supported. */
	break;
      case BS:
	/* Backspace. */
	if (client->active_position_x != 0) {
	  --client->active_position_x;
	}
	break;
      case HT:
	/* Horizontal tab. */
	client->active_position_x = (client->active_position_x + 8) & ~(8-1);
	break;
      case LF:
	/* Line feed. */
	++client->active_position_y;
	if (client->active_position_y == PAGE_LIMIT_POSITION) {
	  scroll (client);
	}
	break;
      case VT:
	/* Vertical tab. */
      case FF:
	/* Form feed. */
	/* Not supported. */
	break;
      case CR:
	/* Carriage return. */
	client->active_position_x = 0;
	break;
      case SO:
      case SI:
      case DLE:
      case DC1:
      case DC2:
      case DC3:
      case DC4:
      case NAK:
      case SYN:
      case ETB:
      case CAN:
      case EM:
      case SUB:
      case ESC:
      case IS4:
      case IS3:
      case IS2:
      case IS1:
	/* Not supported. */
	break;

      case DEL:
	/* Ignore the ASCII delete character. */
	break;

      default:
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

	break;
      }
    }
  }
  
  /* If this is the active client, refresh the VGA. */
  if (client == active_client) {
    refresh = true;
  }
  
  /* TODO:  Remove this line. */
  switch_to_client (client);
}

void
terminal_display (aid_t aid,
		  bd_t bd,
		  void* ptr,
		  size_t buffer_size)
{
  display (aid, ptr, buffer_size);

  schedule ();
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, TERMINAL_DISPLAY, terminal_display);

static bool
terminal_vga_precondition (void)
{
  return refresh && active_client != 0 && terminal_vga_op_binding_count != 0;
}

void
terminal_vga_op (int param,
		 size_t bc)
{
  scheduler_remove (TERMINAL_VGA_OP, param);
  terminal_vga_op_binding_count = bc;

  if (terminal_vga_precondition ()) {
    refresh = false;

    size_t buffer_size = 2 * sizeof (vga_op_t);
    bd_t bd = buffer_create (buffer_size);
    size_t data_offset = buffer_append (bd, active_client->bd, 0, PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE);
    void* ptr = buffer_map (bd);
    buffer_heap_t heap;
    buffer_heap_init (&heap, ptr, buffer_size);
    
    /* Send the data. */
    vga_op_t* assign_op = buffer_heap_alloc (&heap, sizeof (vga_op_t));
    assign_op->type = VGA_ASSIGN;
    assign_op->arg.assign.address = 0;
    assign_op->arg.assign.size = PAGE_LIMIT_POSITION * LINE_LIMIT_POSITION * CELL_SIZE;
    assign_op->arg.assign.data = (buffer_heap_begin (&heap) + data_offset) - (void*)&assign_op->arg.assign.data;
    
    /* Send the cursor. */
    vga_op_t* set_cursor_op = buffer_heap_alloc (&heap, sizeof (vga_op_t));
    assign_op->next = (void*)set_cursor_op - (void*)&assign_op->next;
    set_cursor_op->type = VGA_SET_CURSOR_LOCATION;
    set_cursor_op->arg.set_cursor_location.location = active_client->active_position_y * LINE_LIMIT_POSITION + active_client->active_position_x;
    
    set_cursor_op->next = 0;

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, TERMINAL_VGA_OP, terminal_vga_op);

void
destroyed (aid_t aid,
	   bd_t bd,
	   void* p,
	   size_t buffer_size)
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
  scheduler_finish (bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, DESTROYED, destroyed);

static void
schedule (void)
{
  if (terminal_vga_precondition ()) {
    scheduler_add (TERMINAL_VGA_OP, 0);
  }
}
