#include "ramdisk_automaton.hpp"
#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kput.hpp"
#include "kassert.hpp"

namespace ramdisk {
  
  // Delay initialization.
  static list_alloc alloc_ (false);
  static fifo_scheduler* scheduler_ = 0;
    
  static void
  schedule ()
  {
  }
  
  static void
  finish (void* buffer)
  {
    scheduler_->finish (buffer);
  }
  
  static void
  init_effect (void*,
	       int& value)
  {
    kputs (__func__); kputs (" consumed "); kputx32 (value); kputs ("\n");

    void* tos;
    // Determine the beginning of the stack.
    asm volatile ("mov %%esp, %0\n" : "=m"(tos));

    kputp (tos); kputs ("\n");

    char* x = (char*)sys_allocate (0x1000);

    kassert (0);    

    kputp (x); kputs ("\n");

    // Initialize the allocator.
    new (&alloc_) list_alloc ();
    
    // Allocate a scheduler.
    scheduler_ = new (alloc_) fifo_scheduler (alloc_);
  }

  void
  init (void* p,
	int m)
  {
    input_action <init_traits> (p, m, init_effect, schedule, finish);
  }

  void
  read_request (aid_t aid,
		block_num block_num)
  {
    kputs (__func__); kputs (" aid = "); kputx32 (aid); kputs (" block_num = "); kputx32 (block_num); kputs ("\n");
    kassert (0);
  }

}
