#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "new.hpp"
#include "kassert.hpp"

static list_allocator* allocator = 0;
static fifo_scheduler* scheduler = 0;

UV_UP_INPUT (initrd_init, scheduler);

static void
initrd_init_effect ()
{
  kputs (__func__); kputs ("\n");
  allocator = new list_allocator;
  kassert (allocator != 0);
  scheduler = new (allocator->alloc (sizeof (fifo_scheduler))) fifo_scheduler (*allocator);
}

static void
initrd_init_schedule () {
  /* Do nothing for right now. */
  kputs (__func__); kputs ("\n");
}
