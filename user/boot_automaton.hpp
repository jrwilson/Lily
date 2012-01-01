#ifndef __boot_automaton_hpp__
#define __boot_automaton_hpp__

/*
  File
  ----
  boot_automaton.hpp
  
  Description
  -----------
  The boot automaton creates the first user automaton.

  Authors:
  Justin R. Wilson
*/

#include "action_traits.hpp"

namespace boot {

  // Init.
  typedef np_nb_nc_input_action_traits init_traits;  
  void
  init (void);

  typedef np_nb_nc_output_action_traits create_request_traits;
  void
  create_request (void);

  typedef np_nb_nc_input_action_traits create_response_traits;
  void
  create_response (void);
}

#endif /* __boot_automaton_hpp__ */

