#ifndef __initrd_automaton_hpp__
#define __initrd_automaton_hpp__

/*
  File
  ----
  initrd_automaton.hpp
  
  Description
  -----------
  Declarations for the initial ramdisk.

  Authors:
  Justin R. Wilson
*/

#include "action_macros.hpp"

void
initrd_init (void*,
	     int);
typedef input_action_traits<void*, int, &initrd_init> initrd_init_traits;

#endif /* __initrd_automaton_hpp__ */

