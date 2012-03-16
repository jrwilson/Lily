#include "fifo_scheduler.h"
#include "dymem.h"
#include "automaton.h"

/* TODO:  Optimize. */

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
schedule (void);

static void
scheduler_finish (bool output_fired,
		  bd_t bda,
		  bd_t bdb)
{
  if (item_head != 0) {
    finish (item_head->action_number, item_head->parameter, output_fired, bda, bdb);
  }
  else {
    finish (NO_ACTION, 0, output_fired, bda, bdb);
  }
}

void
end_input_action (bd_t bda,
		  bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  schedule ();
  scheduler_finish (false, -1, -1);
}

void
end_output_action (bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  schedule ();
  scheduler_finish (output_fired, bda, bdb);
}

void
end_internal_action (void)
{
  schedule ();
  scheduler_finish (false, -1, -1);
}
