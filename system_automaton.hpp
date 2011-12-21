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
#include "automaton.hpp"

namespace system_automaton {

  extern automaton* system_automaton;

  typedef p_nb_nc_output_action_traits<automaton*> init_traits;
  void
  init (automaton* p);

}

#endif /* __system_automaton_hpp__ */
