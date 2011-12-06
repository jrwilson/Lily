#ifndef __ramdisk_automaton_hpp__
#define __ramdisk_automaton_hpp__

/*
  File
  ----
  ramdisk_automaton.hpp
  
  Description
  -----------
  Declarations for a ramdisk.

  Authors:
  Justin R. Wilson
*/

#include "action_macros.hpp"

namespace ramdisk {

  typedef uint64_t block_num;
  
  void
  init (void*,
	int);
  typedef p_v_input_action_traits<void*, int> init_traits;

  void
  read_request (aid_t,
		block_num);
  typedef ap_v_input_action_traits<block_num> read_request_traits;
}

#endif /* __ramdisk_automaton_hpp__ */

