#include "initrd_automaton.hpp"
#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kput.hpp"
#include "kassert.hpp"

// Delay initialization.
static list_alloc alloc_ (false);
static fifo_scheduler* scheduler_ = 0;

static void
initrd_init_effect (void*,
		    int& value)
{
  // Initialize the allocator.
  new (&alloc_) list_alloc ();

  // Allocate a scheduler.
  scheduler_ = new (alloc_) fifo_scheduler (alloc_);
  
  kputs (__func__); kputs (" consumed "); kputx32 (value); kputs ("\n");

  char* x = new (alloc_) char[4096];
  kputs ("x = "); kputp (x); kputs ("\n");
}

static void
initrd_schedule ()
{
}

static void
initrd_finish (void* buffer)
{
  scheduler_->finish (buffer);
}

void
initrd_init (void* p,
	     int m)
{
  input_action <initrd_init_traits> (p, m, initrd_init_effect, initrd_schedule, initrd_finish);
}
