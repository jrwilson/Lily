#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kput.hpp"

// Delay initialization.
static list_alloc alloc_ (false);
static fifo_scheduler* scheduler_ = 0;

static void
initrd_init_effect ()
{
  // Initialize the allocator.
  new (&alloc_) list_alloc ();

  // Allocate a scheduler.
  scheduler_ = new (alloc_) fifo_scheduler (alloc_);

  char* x = new (alloc_) char[4096];
  kputs ("x = "); kputp (x); kputs ("\n");

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
  scheduler_->finish (output_status, output_value);
}

UV_UP_INPUT (initrd_init);
