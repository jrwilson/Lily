#include "fifo_scheduler.h"
#include "kput.h"

static fifo_scheduler_t* scheduler = 0;

static void
schedule ();

static int
consumer_init_precondition ()
{
  return scheduler == 0;
}

static void
consumer_init_effect ()
{
  scheduler = allocate_fifo_scheduler ();
}

static void
consumer_init_schedule () {
  schedule ();
}

UP_INTERNAL (consumer_init);

static void
consumer_consume_effect (unsigned int value)
{
  kputs ("consumer "); kputuix (value); kputs ("\n");
}

static void
consumer_consume_schedule () {
  schedule ();
}

V_UP_INPUT (consumer_consume);

static void
schedule ()
{
  if (consumer_init_precondition ()) {
    SCHEDULER_ADD (scheduler, (unsigned int)&consumer_init_entry, 0);
  }
}
