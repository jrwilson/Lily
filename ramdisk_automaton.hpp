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
  typedef input_action_traits<void*, int, &init> init_traits;

  void
  read_request (aid_t,
		block_num);
  typedef input_action_traits<aid_t, block_num, &read_request> read_request_traits;
}

#endif /* __ramdisk_automaton_hpp__ */

