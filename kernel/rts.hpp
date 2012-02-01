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
	  ano_t action_number,
	  const void* parameter,
	  bd_t bd,
	  size_t buffer_size,
	  int flags);
  
  pair<aid_t, int>
  create (automaton* a,
	  bd_t text_bd,
	  size_t text_buffer_size,
	  bool retain_privilege,
	  bd_t data_bd,
	  size_t data_buffer_size);

  pair<bid_t, int>
  bind (automaton* a,
	aid_t output_automaton,
	ano_t output_action,
	const void* output_parameter,
	aid_t input_automaton,
	ano_t input_action,
	const void* input_parameter);

  void
  loose (void);

  void
  destroy (void);

  pair<int, int>
  subscribe_destroyed (automaton* a,
		       aid_t aid);

  pair<int, int>
  map (automaton* a,
       const void* destination,
       const void* source,
       size_t size);

  pair<int, int>
  reserve_port (automaton* a,
		unsigned short port);
}

#endif /* __rts_hpp__ */
