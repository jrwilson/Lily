#include "producer.h"
#include <automaton.h>
#include <string.h>
#include <buffer_heap.h>
#include "terminal.h"
#include "keyboard.h"
#include <fifo_scheduler.h>
#include <dymem.h>

typedef struct item item_t;
struct item {
  bd_t bd;
  item_t* next;
};

static item_t* head = 0;
static item_t** tail = &head;
static size_t producer_display_binding_count = 1;

static bool
empty (void)
{
  return head == 0;
}

static void
push (item_t* item)
{
  item->next = 0;
  *tail = item;
  tail = &item->next;
}

static item_t*
pop (void)
{
  item_t* retval = head;
  head = retval->next;
  if (head == 0) {
    tail = &head;
  }
  return retval;
}

static void
schedule (void);

void
producer_string (int param,
		 bd_t bd,
		 void* ptr,
		 size_t size)
{
  if (bd != -1) {
    item_t* item = malloc (sizeof (item_t));
    item->bd = bd;
    push (item);
  }

  schedule ();
  scheduler_finish (bd, FINISH_NO);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, PRODUCER_STRING, producer_string);

static bool
producer_display_precondition (void)
{
  return !empty () && producer_display_binding_count != 0;
}

void
producer_display (int param,
		  size_t bc)
{
  scheduler_remove (PRODUCER_DISPLAY, param);
  producer_display_binding_count = bc;

  if (producer_display_precondition ()) {
    item_t* item = pop ();
    bd_t bd = item->bd;
    free (item);

    schedule ();
    scheduler_finish (bd, FINISH_DESTROY);
  }
  else {
    schedule ();
    scheduler_finish (-1, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_DISPLAY, producer_display);

static void
schedule (void)
{
  if (producer_display_precondition ()) {
    scheduler_add (PRODUCER_DISPLAY, 0);
  }
}
