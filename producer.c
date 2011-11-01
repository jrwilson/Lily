#include "fifo_scheduler.h"

static fifo_scheduler_t* scheduler = 0;
static unsigned int counter = 0;

static void
schedule ();

static int
producer_init_precondition ()
{
  return scheduler == 0;
}

static void
producer_init_effect ()
{
  scheduler = allocate_fifo_scheduler ();
}

static void
producer_init_schedule () {
  schedule ();
}

UP_INTERNAL (producer_init);

static int
producer_produce_precondition ()
{
  return 1;
}

static unsigned int
producer_produce_effect ()
{
  return counter++;
}

static void
producer_produce_schedule () {
  schedule ();
}

V_UP_OUTPUT (producer_produce);

static void
schedule ()
{
  if (producer_init_precondition ()) {
    SCHEDULER_ADD (scheduler, (unsigned int)&producer_init_entry, 0);
  }
  if (producer_produce_precondition ()) {
    SCHEDULER_ADD (scheduler, (unsigned int)&producer_produce_entry, 0);
  }
}
