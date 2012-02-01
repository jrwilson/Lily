#include "console.h"
#include "vga.h"
#include <automaton.h>
#include <string.h>
#include <dymem.h>

/* TODO:  Factor this out into a library and optimize it. */

typedef struct item item_t;
struct item {
  ano_t action_number;
  int parameter;
  item_t* next;
};

static item_t* item_head = 0;

void
scheduler_add (ano_t action_number,
	       int parameter)
{
  /* Scan the list. */
  item_t** ptr;
  for (ptr = &item_head;
       *ptr != 0 && !((*ptr)->action_number == action_number && (*ptr)->parameter == parameter);
       ptr = &(*ptr)->next) ;;

  if (*ptr == 0) {
    /* Not in the list.  Insert. */
    item_t* item = malloc (sizeof (item_t));
    item->action_number = action_number;
    item->parameter = parameter;
    item->next = 0;
    *ptr = item;
  }
}

void
scheduler_remove (ano_t action_number,
		  int parameter)
{
  /* Scan the list. */
  item_t** ptr;
  for (ptr = &item_head;
       *ptr != 0 && !((*ptr)->action_number == action_number && (*ptr)->parameter == parameter);
       ptr = &(*ptr)->next) ;;

  if (*ptr != 0) {
    /* In the list.  Remove. */
    item_t* item = *ptr;
    *ptr = item->next;
    free (item);
  }
}

void
scheduler_finish (bd_t bd,
		  size_t buffer_size,
		  int flags)
{
  if (item_head != 0) {
    finish (item_head->action_number, item_head->parameter, bd, buffer_size, flags);
  }
  else {
    finish (NO_ACTION, 0, bd, buffer_size, flags);
  }
}




#define DISPLAY_PAGE_COUNT 8
#define DISPLAY_PAGE_SIZE 4096

typedef struct display_page display_page_t;
typedef struct client_context client_context_t;

struct display_page {
  /* Offset in display memory. */
  size_t offset;
  /* Client currently using this display page. */
  client_context_t* client;
  /* The pages form a list where the front contains the least recently activated page. */
  display_page_t* next;
  display_page_t* prev;
};

struct client_context {
  /* Associated aid. */
  aid_t aid;
  /* Buffer descriptor for the data associated with this client. */
  bd_t bd;
  /* Same only mapped in. */
  unsigned char* buffer;
  /* Display page allocated to this client. */
  display_page_t* display_page;
  /* Next on the list. */
  client_context_t* next;
};

typedef struct vga_op_item vga_op_item_t;
struct vga_op_item {
  bd_t bd;
  size_t buffer_size;
  vga_op_item_t* next;
};

static bool initialized = false;
static display_page_t display_page[DISPLAY_PAGE_COUNT];
static display_page_t* display_page_queue_front = 0;
static display_page_t* display_page_queue_back = 0;
static client_context_t* context_list_head = 0;
static vga_op_item_t* vga_op_queue_front = 0;
static vga_op_item_t** vga_op_queue_back = &vga_op_queue_front;
/* Non-zero to start. */
static size_t console_vga_op_binding_count = 1;

/* We assume the queue is constant size and non-empty.
   Thus, the front and back are always valid and never equal. */

static void
display_page_queue_remove (display_page_t* dp)
{
  display_page_t* prev = dp->prev;
  display_page_t* next = dp->next;

  if (prev != 0) {
    prev->next = next;
  }

  if (next != 0) {
    next->prev = prev;
  }
   
  dp->next = 0;
  dp->prev = 0;
}

static void
display_page_queue_push_front (display_page_t* dp)
{
  display_page_queue_front->prev = dp;
  dp->next = display_page_queue_front;
  dp->prev = 0;
  display_page_queue_front = dp;
}

static display_page_t*
display_page_queue_pop_front (void)
{
  display_page_t* retval = display_page_queue_front;
  display_page_queue_front = display_page_queue_front->next;
  display_page_queue_front->prev = 0;
  retval->next = 0;

  return retval;
}

static void
display_page_queue_push_back (display_page_t* dp)
{
  display_page_queue_back->next = dp;
  dp->prev = display_page_queue_back;
  dp->next = 0;
  display_page_queue_back = dp;
}

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
vga_set_start (size_t offset)
{

  vga_op_item_t* item = malloc (sizeof (vga_op_item_t));
  size_t buffer_size = sizeof (vga_op_t);
  item->bd = buffer_create (buffer_size);
  item->buffer_size = buffer_size;
  vga_op_t* op = buffer_map (item->bd);
  op->type = VGA_SET_START;
  /* The odd/even operation of the VGA in alphanumeric mode requires us to use a start address in the lower half of its memory. */
  op->arg.set_start.offset = offset / 2;

  vga_op_queue_push (item);
}

static void
vga_assign (size_t offset,
	    const void* data,
	    size_t size)
{
  vga_op_item_t* item = malloc (sizeof (vga_op_item_t));
  size_t buffer_size = sizeof (vga_op_t) + size;
  item->bd = buffer_create (buffer_size);
  item->buffer_size = buffer_size;
  vga_op_t* op = buffer_map (item->bd);
  op->type = VGA_ASSIGN;
  op->arg.assign.offset = offset;
  op->arg.assign.size = size;
  memcpy (&op->arg.assign.data[0], data, size);

  vga_op_queue_push (item);
}

static void
vga_copy (size_t dst_offset,
	  size_t src_offset,
	  size_t size)
{
  vga_op_item_t* item = malloc (sizeof (vga_op_item_t));
  size_t buffer_size = sizeof (vga_op_t);
  item->bd = buffer_create (buffer_size);
  item->buffer_size = buffer_size;
  vga_op_t* op = buffer_map (item->bd);
  op->type = VGA_COPY;
  op->arg.copy.dst_offset = dst_offset;
  op->arg.copy.src_offset = src_offset;
  op->arg.copy.size = size;

  vga_op_queue_push (item);
}

static void
initialize (void)
{
  if (!initialized) {
    display_page_queue_front = &display_page[0];

    display_page[0].offset = 0;
    display_page[0].client = 0;
    display_page[0].next = &display_page[1];
    display_page[0].prev = 0;

    for (size_t page = 1; page != (DISPLAY_PAGE_COUNT - 1); ++page) {
      display_page[page].offset = page * DISPLAY_PAGE_SIZE;
      display_page[page].client = 0;
      display_page[page].next = &display_page[page + 1];
      display_page[page].prev = &display_page[page - 1];
    }

    display_page[DISPLAY_PAGE_COUNT - 1].offset = (DISPLAY_PAGE_COUNT - 1) * DISPLAY_PAGE_SIZE;
    display_page[DISPLAY_PAGE_COUNT - 1].client = 0;
    display_page[DISPLAY_PAGE_COUNT - 1].next = 0;
    display_page[DISPLAY_PAGE_COUNT - 1].prev = &display_page[DISPLAY_PAGE_COUNT - 2];
    
    display_page_queue_back = &display_page[DISPLAY_PAGE_COUNT - 1];

    initialized = true;
  }
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
  context->bd = buffer_create (DISPLAY_PAGE_SIZE);
  context->buffer = buffer_map (context->bd);
  context->display_page = 0;
  context->next = context_list_head;
  context_list_head = context;

  subscribe_destroyed (aid);

  return context;
}

static void
destroy_client_context (client_context_t* context)
{
  if (context->display_page != 0) {
    /* Client is no longer associated with the display page. */
    context->display_page->client = 0;
    /* Display page can be used immediately. */
    display_page_queue_remove (context->display_page);
    display_page_queue_push_front (context->display_page);
  }
  buffer_destroy (context->bd);
  free (context);
}

static void
switch_to_context (client_context_t* context)
{
  if (context->display_page != 0) {
    /* Cache hit. */
    vga_set_start (context->display_page->offset);
  }
  else {
    display_page_t* display_page = display_page_queue_pop_front ();
    if (display_page->client != 0) {
      /* The display page is associated with a client.
	 Tell the client to forget about it. */
      display_page->client->display_page = 0;
    }

    /* Associate with the display page. */
    context->display_page = display_page;
    display_page->client = context;

    /* Replace. */
    display_page_queue_push_back (display_page);

    vga_assign (context->display_page->offset, context->buffer, DISPLAY_PAGE_SIZE);
    vga_set_start (context->display_page->offset);
  }
}

static void
assign (client_context_t* context,
	size_t offset,
	const void* data,
	size_t size)
{
  if (offset < DISPLAY_PAGE_SIZE &&
      offset + size <= DISPLAY_PAGE_SIZE) {
    memcpy (context->buffer + offset, data, size);
    if (context->display_page != 0) {
      /* Write through. */
      /* TODO:  Maybe we should only write through if writing to the visible page. */
      vga_assign (context->display_page->offset + offset, data, size);
    }
  }
}

static void
copy (client_context_t* context,
      size_t dst_offset,
      size_t src_offset,
      size_t size)
{
  if (dst_offset < DISPLAY_PAGE_SIZE &&
      dst_offset + size <= DISPLAY_PAGE_SIZE &&
      src_offset < DISPLAY_PAGE_SIZE &&
      src_offset + size <= DISPLAY_PAGE_SIZE) {
    memmove (context->buffer + dst_offset, context->buffer + src_offset, size);
    if (context->display_page != 0) {
      /* Write through. */
      /* TODO:  Maybe we should only write through if writing to the visible page. */
      vga_copy (context->display_page->offset + dst_offset, context->display_page->offset + src_offset, size);
    }
  }
}

static void schedule (void);

void
init (int param,
      bd_t bd,
      size_t buffer_size,
      void* ptr)
{
  initialize ();
  schedule ();
  scheduler_finish (bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

typedef struct {
  aid_t aid;
} focus_arg_t;

void
console_focus (int param,
	   bd_t bd,
	   size_t buffer_size,
	   focus_arg_t* a)
{
  initialize ();

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
  initialize ();

  if (op != 0 && buffer_size >= sizeof (console_op_t)) {
    client_context_t* context = find_client_context (aid);
    if (context == 0) {
      context = create_client_context (aid);
    }

    /* TODO:  Remove this line. */
    switch_to_context (context);

    switch (op->type) {
    case CONSOLE_ASSIGN:
      if (sizeof (console_op_t) + op->arg.assign.size == buffer_size) {
    	assign (context, op->arg.assign.offset, op->arg.assign.data, op->arg.assign.size);
      }
      break;
    case CONSOLE_COPY:
      copy (context, op->arg.copy.dst_offset, op->arg.copy.src_offset, op->arg.copy.size);
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
  initialize ();

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
