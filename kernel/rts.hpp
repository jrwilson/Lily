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
	  int parameter,
	  bool output_fired,
	  bd_t bda,
	  bd_t bdb);
  
  pair<aid_t, int>
  create (automaton* a,
	  bd_t text_bd,
	  size_t text_size,
	  bd_t bda,
	  bd_t bdb,
	  bool retain_privilege);

  pair<bid_t, int>
  bind (automaton* a,
	aid_t output_automaton,
	ano_t output_action,
	int output_parameter,
	aid_t input_automaton,
	ano_t input_action,
	int input_parameter);

  void
  loose (void);

  void
  destroy (void);

  pair<int, int>
  subscribe_destroyed (automaton* a,
		       ano_t action_number,
		       aid_t aid);

  pair<int, int>
  enter (automaton* a,
	 const char* name,
	 size_t size,
	 aid_t aid);

  pair<aid_t, int>
  lookup (automaton* a,
	  const char* name,
	  size_t size);

  pair<bd_t, int>
  describe (automaton* a,
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
