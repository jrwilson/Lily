#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kput.hpp"

static void
initrd_init_effect ()
{
  kputs (__func__); kputs ("\n");
}

static void
initrd_init_schedule () {
  kputs (__func__); kputs ("\n");
}

static void
schedule_finish (bool output_status,
		 value_t output_value)
{
  kputs (__func__); kputs ("\n");
  //scheduler_->finish (output_status, output_value);

  sys_finish (false, 0, 0, false, 0);
}

UV_UP_INPUT (initrd_init);
