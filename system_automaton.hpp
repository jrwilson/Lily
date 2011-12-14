#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

/*
  File
  ----
  system_automaton.hpp
  
  Description
  -----------
  Declarations for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include "action_traits.hpp"

class automaton;

namespace system_automaton {

  typedef p_uv_output_action_traits<automaton*> init_traits;
  void
  init (automaton* p);

}

#endif /* __system_automaton_hpp__ */
