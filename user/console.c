#include "console.h"
#include "vga.h"
#include <automaton.h>
#include <string.h>
#include <dymem.h>
#include <fifo_scheduler.h>

/* Number of rows. */
#define HEIGHT 25

/* Number of columns. */
#define WIDTH_CHAR 80

/* Number of columns in bytes.  The 2 comes from the fact that each character is two bytes:  one for the character and one for the attributes. */
#define WIDTH_BYTE (2 * WIDTH_CHAR)

/* Number of bytes in one screen. */
#define SCREEN_SIZE_BYTE (HEIGHT * WIDTH_BYTE)

/* The VGA has 32,768 bytes of memory.
   The maximum number of lines 204 = 32,768 / WIDTH_BYTE.
   The largest address is then 204 * WIDTH_BYTE = 32,640.
 */
#define VGA_ADDRESS_LIMIT 32640

typedef struct client_context client_context_t;
struct client_context {
  /* Associated aid. */
  aid_t aid;
  /* Buffer descriptor for the data associated with this client. */
  bd_t bd;
  /* Same only mapped in. */
  unsigned char* buffer;
  /* The current line.  This is the last line displayed. */
  size_t line;
  /* Next on the list. */
  client_context_t* next;
};

typedef struct vga_op_item vga_op_item_t;
struct vga_op_item {
  bd_t bd;
  size_t buffer_size;
  vga_op_item_t* next;
};

static client_context_t* context_list_head = 0;
static client_context_t* active_client = 0;
static vga_op_item_t* vga_op_queue_front = 0;
static vga_op_item_t** vga_op_queue_back = &vga_op_queue_front;
/* Non-zero to start. */
static size_t console_vga_op_binding_count = 1;

/* The address where data will be written to the VGA. */
static size_t vga_address = SCREEN_SIZE_BYTE;

static bool
vga_op_queue_empty (void)
{
  return vga_op_queue_front == 0;
}

static void
vga_op_queue_push (vga_op_item_t* item)
{
  *vga_op_queue_back = item;
  vga_op_queue_back = &item->next;
  *vga_op_queue_back = 0;
}

static vga_op_item_t*
vga_op_queue_pop (void)
{
  vga_op_item_t* retval = vga_op_queue_front;
  vga_op_queue_front = vga_op_queue_front->next;
  if (vga_op_queue_front == 0) {
    vga_op_queue_back = &vga_op_queue_front;
  }
  return retval;
}

static void
vga_set_start_address (size_t address)
{

  vga_op_item_t* item = malloc (sizeof (vga_op_item_t));
  size_t buffer_size = sizeof (vga_op_t);
  item->bd = buffer_create (buffer_size);
  item->buffer_size = buffer_size;
  vga_op_t* op = buffer_map (item->bd);
  op->type = VGA_SET_START_ADDRESS;
  op->arg.set_start_address.address = address;

  vga_op_queue_push (item);
}

static void
vga_assign (size_t address,
	    const void* data,
	    size_t size)
{
  vga_op_item_t* item = malloc (sizeof (vga_op_item_t));
  size_t buffer_size = sizeof (vga_op_t) + size;
  item->bd = buffer_create (buffer_size);
  item->buffer_size = buffer_size;
  vga_op_t* op = buffer_map (item->bd);
  op->type = VGA_ASSIGN;
  op->arg.assign.address = address;
  op->arg.assign.size = size;
  memcpy (&op->arg.assign.data[0], data, size);

  vga_op_queue_push (item);
}

static client_context_t*
find_client_context (aid_t aid)
{
  client_context_t* ptr;
  for (ptr = context_list_head; ptr != 0 && ptr->aid != aid; ptr = ptr->next) ;;
  return ptr;
}

static client_context_t*
create_client_context (aid_t aid)
{
  client_context_t* context = malloc (sizeof (client_context_t));
  context->aid = aid;
  context->bd = buffer_create (SCREEN_SIZE_BYTE);
  context->buffer = buffer_map (context->bd);
  context->line = 0;
  context->next = context_list_head;
  context_list_head = context;

  subscribe_destroyed (aid);

  return context;
}

static void
destroy_client_context (client_context_t* context)
{
  if (context == active_client) {
    /* TODO:  Perhaps we should blank the screen. */
    active_client = 0;
  }
  buffer_destroy (context->bd);
  free (context);
}

static void
send_lines (client_context_t* context,
	    size_t first_line,
	    size_t count)
{
  if (count != 0) {
    if (vga_address + count * WIDTH_BYTE > VGA_ADDRESS_LIMIT) {
      /* Restart. */
      vga_address = SCREEN_SIZE_BYTE;
      first_line = (context->line + 1) % HEIGHT;
      count = HEIGHT;
    }

    if (first_line + count <= HEIGHT) {
      /* One step. */
      vga_assign (vga_address, context->buffer + first_line * WIDTH_BYTE, WIDTH_BYTE * count);
      vga_address += WIDTH_BYTE * count;
    }
    else {
      /* Two steps. */
      const size_t count1 = (HEIGHT - first_line);
      const size_t count2 = count - count1;
      vga_assign (vga_address, context->buffer + first_line * WIDTH_BYTE, WIDTH_BYTE * count1);
      vga_address += WIDTH_BYTE * count1;
      vga_assign (vga_address, context->buffer, WIDTH_BYTE * count2);
      vga_address += WIDTH_BYTE * count2;
    }
    
    vga_set_start_address ((vga_address - SCREEN_SIZE_BYTE) / 2);
  }
}

static void
switch_to_context (client_context_t* context)
{
  if (context != active_client) {
    /* Send all of the lines. */
    send_lines (context, (context->line + 1) % HEIGHT, HEIGHT);
    active_client = context;
  }
}

static void
append (client_context_t* context,
	const size_t line_count,
	const unsigned char* data)
{
  if (line_count < HEIGHT) {
    size_t first_line = (context->line + 1) % HEIGHT;
    for (size_t count = line_count; count != 0; --count, data += WIDTH_BYTE) {
      context->line = (context->line + 1) % HEIGHT;
      memcpy (context->buffer + context->line * WIDTH_BYTE, data, WIDTH_BYTE);
    }

    if (context == active_client) {
      send_lines (context, first_line, line_count);
    }
  }
  else {
    /* Start over with the last HEIGHT lines. */
    data += (line_count - HEIGHT) * WIDTH_BYTE;
    memcpy (context->buffer, data, SCREEN_SIZE_BYTE);
    context->line = HEIGHT - 1;
    if (context == active_client) {
      send_lines (context, 0, HEIGHT);
    }
  }
}

static void
replace_last (client_context_t* context,
	      const void* data)
{
  memcpy (context->buffer + context->line * WIDTH_BYTE, data, WIDTH_BYTE);

  if (context == active_client) {
    vga_assign (vga_address - WIDTH_BYTE, data, WIDTH_BYTE);
  }
}

static void schedule (void);

typedef struct {
  aid_t aid;
} focus_arg_t;

void
console_focus (int param,
	   bd_t bd,
	   size_t buffer_size,
	   focus_arg_t* a)
{
  if (a != 0 && buffer_size == sizeof (focus_arg_t)) {
    client_context_t* context = find_client_context (a->aid);
    if (context != 0) {
      switch_to_context (context);
    }
  }

  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, CONSOLE_FOCUS, console_focus);

void
console_op (aid_t aid,
	    bd_t bd,
	    size_t buffer_size,
	    console_op_t* op)
{
  if (op != 0 && buffer_size >= sizeof (console_op_t)) {
    client_context_t* context = find_client_context (aid);
    if (context == 0) {
      context = create_client_context (aid);
    }

    /* TODO:  Remove this line. */
    switch_to_context (context);

    switch (op->type) {
    case CONSOLE_APPEND:
      if (buffer_size >= sizeof (console_op_t) + op->arg.append.line_count * WIDTH_BYTE) {
    	append (context, op->arg.append.line_count, op->arg.append.data);
      }
      break;
    case CONSOLE_REPLACE_LAST:
      if (buffer_size >= sizeof (console_op_t) + WIDTH_BYTE) {
    	replace_last (context, op->arg.replace_last.data);
      }
      break;
    }
  }

  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, AUTO_PARAMETER, CONSOLE_OP, console_op);

static bool
console_vga_precondition (void)
{
  return !vga_op_queue_empty () && console_vga_op_binding_count != 0;
}

void
console_vga_op (int param,
		size_t bc)
{
  scheduler_remove (CONSOLE_VGA_OP, param);
  console_vga_op_binding_count = bc;

  if (console_vga_precondition ()) {
    vga_op_item_t* item = vga_op_queue_pop ();
    bd_t bd = item->bd;
    size_t buffer_size = item->buffer_size;
    free (item);
    schedule ();
    scheduler_finish (bd, buffer_size, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, 0, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, CONSOLE_VGA_OP, console_vga_op);

void
destroyed (aid_t aid,
	   bd_t bd,
	   size_t buffer_size,
	   void* p)
{
  /* Destroy the client context. */
  client_context_t** ptr = &context_list_head;
  for (; *ptr != 0 && (*ptr)->aid != aid; ptr = &(*ptr)->next) ;;
  if (*ptr != 0) {
    client_context_t* temp = *ptr;
    *ptr = temp->next;
    destroy_client_context (temp);
  }

  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, PARAMETER, DESTROYED, destroyed);

static void
schedule (void)
{
  if (console_vga_precondition ()) {
    scheduler_add (CONSOLE_VGA_OP, 0);
  }
}
