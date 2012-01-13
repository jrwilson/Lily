#ifndef __rts_hpp__
#define __rts_hpp__

/*
  File
  ----
  rts.hpp
  
  Description
  -----------
  Run-time system.

  Authors:
  Justin R. Wilson
*/

#include "vm_def.hpp"
#include "aid.hpp"
#include "bid.hpp"

namespace rts {

  void
  create_init_automaton (frame_t automaton_frame,
			 size_t automaton_size,
			 frame_t data_frame,
			 size_t data_size);
  
  aid_t
  create (bid_t automaton_bid,
	  size_t automaton_size);

  void
  bind (void);

  void
  loose (void);

  void
  destroy (void);
}

#endif /* __rts_hpp__ */
