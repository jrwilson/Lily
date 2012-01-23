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
#include "lily/types.h"
#include "action.hpp"
#include "utility.hpp"

namespace rts {

  void
  create_init_automaton (frame_t automaton_frame,
			 size_t automaton_size,
			 frame_t data_frame,
			 size_t data_size);

  void
  finish (const caction& current,
	  const void* action_entry_point,
	  const void* parameter,
	  const void* value,
	  size_t value_size,
	  bid_t buffer,
	  size_t buffer_size);
  
  aid_t
  create (const void* automaton_buffer,
	  size_t automaton_size);

  void
  bind (void);

  void
  loose (void);

  void
  destroy (void);

  pair<int, int>
  map (automaton* a,
       const void* destination,
       const void* source,
       size_t size);
}

#endif /* __rts_hpp__ */
