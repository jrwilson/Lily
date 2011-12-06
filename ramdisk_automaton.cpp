#include "ramdisk_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "action_macros.hpp"
#include "kput.hpp"
#include "kassert.hpp"

namespace ramdisk {
  
  // Delay initialization.
  static list_alloc alloc_ (false);
  typedef fifo_scheduler<list_alloc, list_allocator> scheduler_type;
  static scheduler_type* scheduler_ = 0;
    
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
    
    // Initialize the allocator.
    new (&alloc_) list_alloc ();
    
    // Allocate a scheduler.
    scheduler_ = new (alloc_) scheduler_type (alloc_);
  }

  void
  init (void* p,
	int v)
  {
    input_action <init_traits> (p, v, init_effect, schedule, finish);
  }

  void
  read_request (aid_t aid,
		block_num block_num)
  {
    kputs (__func__); kputs (" aid = "); kputx32 (aid); kputs (" block_num = "); kputx32 (block_num); kputs ("\n");
    kassert (0);
  }

}
