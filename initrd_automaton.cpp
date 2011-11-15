#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "new.hpp"
#include "kassert.hpp"

static fifo_scheduler* scheduler = 0;

UV_UP_INPUT (initrd_init, scheduler);

static void
initrd_init_effect ()
{
  kputs (__func__); kputs ("\n");
}

static void
initrd_init_schedule () {
  kputs (__func__); kputs ("\n");
}
