#include "fifo_scheduler.hpp"
#include "kassert.hpp"

static list_allocator_t* list_allocator = 0;
static fifo_scheduler_t* scheduler = 0;

UV_UP_INPUT (initrd_init);

static void
initrd_init_effect ()
{
  kputs (__func__); kputs ("\n");
  list_allocator = list_allocator_allocate ();
  scheduler = fifo_scheduler_allocate (list_allocator);
}

static void
initrd_init_schedule () {
  /* Do nothing for right now. */
}
