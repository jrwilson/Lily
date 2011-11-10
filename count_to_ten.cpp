#include "fifo_scheduler.h"
#include "kput.h"

static fifo_scheduler_t* scheduler = 0;
static unsigned int counter = 0;

static void
schedule ();

static int
init_precondition ()
{
  return scheduler == 0;
}

static void
init_effect ()
{
  kputs (__func__); kputs ("\n");
  scheduler = allocate_fifo_scheduler ();
}

static void
init_schedule () {
  schedule ();
}

UP_INTERNAL (init);

static int
increment_precondition ()
{
  return counter < 10;
}

static void
increment_effect ()
{
  kputs ("counter = "); kputuix (counter); kputs ("\n");
  ++counter;
}

static void
increment_schedule () {
  schedule ();
}

UP_INTERNAL (increment);

static void
schedule ()
{
  if (init_precondition ()) {
    SCHEDULER_ADD (scheduler, (unsigned int)&init_entry, 0);
  }
  if (increment_precondition ()) {
    SCHEDULER_ADD (scheduler, (unsigned int)&increment_entry, 0);
  }
}
