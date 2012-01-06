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

#include "automaton.hpp"

namespace rts {

  extern automaton* system_automaton;

  extern bid_t automaton_bid;
  extern size_t automaton_size;
  extern bid_t data_bid;
  extern size_t data_size;

  void
  create_system_automaton (buffer* automaton_buffer,
			   size_t automaton_size,
			   buffer* data_buffer,
			   size_t data_size);

  void
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
