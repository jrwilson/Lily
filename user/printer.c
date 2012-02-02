#include "printer.h"
#include <automaton.h>
#include <fifo_scheduler.h>
#include <dymem.h>
#include "console.h"
#include <string.h>

/* Number of columns. */
#define WIDTH_CHAR 80

/* Number of columns in bytes.  The 2 comes from the fact that each character is two bytes:  one for the character and one for the attributes. */
#define WIDTH_BYTE (2 * WIDTH_CHAR)

/* Number of lines per message. */
#define LINES_PER_MESSAGE 25

/* White on black. */
#define ATTRIBUTE 0x0F00

typedef struct console_op_item console_op_item_t;
struct console_op_item {
  bd_t bd;
  size_t buffer_size;
  console_op_item_t* next;
};

static console_op_item_t* console_op_queue_front = 0;
static console_op_item_t** console_op_queue_back = &console_op_queue_front;
/* Non-zero to start. */
static size_t printer_console_op_binding_count = 1;
static size_t x_location = 0;
static console_op_item_t* item = 0;
static console_op_t* op;
static unsigned short* line_data;
static size_t max_line_count;
static size_t current_line;
static unsigned short last_line[WIDTH_CHAR];

static bool
console_op_queue_empty (void)
{
  return console_op_queue_front == 0;
}

static void
console_op_queue_push (console_op_item_t* item)
{
  *console_op_queue_back = item;
  console_op_queue_back = &item->next;
  *console_op_queue_back = 0;
}

static console_op_item_t*
console_op_queue_pop (void)
{
  console_op_item_t* retval = console_op_queue_front;
  console_op_queue_front = console_op_queue_front->next;
  if (console_op_queue_front == 0) {
    console_op_queue_back = &console_op_queue_front;
  }
  return retval;
}

static void
finalize (void)
{
  if (op->type == CONSOLE_APPEND) {
    op->arg.append.line_count = current_line + 1;
  }

  console_op_queue_push (item);
  item = 0;
}

static void
put (const char c)
{
  if (item == 0) {
    /* Start a new item. */
    item = malloc (sizeof (console_op_item_t));
    size_t buffer_size;
    if (x_location == 0) {
      /* Append. */
      buffer_size = sizeof (console_op_t) + WIDTH_BYTE * LINES_PER_MESSAGE;
    }
    else {
      /* Replace. */
      buffer_size = sizeof (console_op_t) + WIDTH_BYTE;
    }

    item->bd = buffer_create (buffer_size);
    item->buffer_size = buffer_size;
    op = buffer_map (item->bd);
    if (x_location == 0) {
      op->type = CONSOLE_APPEND;
      line_data = (unsigned short*)op->arg.append.data;
      max_line_count = LINES_PER_MESSAGE;
    }
    else {
      op->type = CONSOLE_REPLACE_LAST;
      line_data = (unsigned short*)op->arg.replace_last.data;
      memcpy (line_data, last_line, x_location * sizeof (unsigned short));
      max_line_count = 1;
    }
    current_line = 0;

    item->next = 0;
  }

  switch (c) {
  case '\b':
    if (x_location != 0) {
      --x_location;
    }
    break;
  case '\t':
    /* A tab is a position divisible by 8. */
    x_location = (x_location + 8) & ~(8-1);
    break;
  case '\n':
    x_location = 0;
    ++current_line;
    memset (last_line, 0, WIDTH_BYTE);
    break;
  case '\r':
    x_location = 0;
    break;
  default:
    line_data[current_line * WIDTH_CHAR + x_location] = ATTRIBUTE | c;
    last_line[x_location] = ATTRIBUTE | c;
    ++x_location;
    if (x_location == WIDTH_CHAR) {
      x_location = 0;
      ++current_line;
      memset (last_line, 0, WIDTH_BYTE);
    }
  }
  
  if (current_line == max_line_count) {
    finalize ();
  }
}

static void schedule (void);

void
printer_print (aid_t aid,
	       bd_t bd,
	       const size_t buffer_size,
	       const char* begin)
{
  if (begin != 0) {
    const char* end = begin + buffer_size;
    for (; begin != end; ++begin) {
      put (*begin);
    }

    /* Insert into the queue. */
    if (item != 0) {
      finalize ();
    }
  }

  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, PRINTER_PRINT, printer_print);

static bool
printer_console_precondition (void)
{
  return !console_op_queue_empty () && printer_console_op_binding_count != 0;
}

void
printer_console_op (int param,
		    size_t bc)
{
  scheduler_remove (PRINTER_CONSOLE_OP, param);
  printer_console_op_binding_count = bc;

  if (printer_console_precondition ()) {
    console_op_item_t* item = console_op_queue_pop ();
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
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRINTER_CONSOLE_OP, printer_console_op);

static void
schedule (void)
{
  if (printer_console_precondition ()) {
    scheduler_add (PRINTER_CONSOLE_OP, 0);
  }
}
