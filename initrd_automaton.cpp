#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kassert.hpp"

static fifo_scheduler* scheduler = 0;

UV_UP_INPUT (initrd_init, *scheduler);

static void
initrd_init_effect ()
{
  scheduler = new fifo_scheduler ();
  kputs (__func__); kputs ("\n");
}

static void
initrd_init_schedule () {
  kputs (__func__); kputs ("\n");
}
