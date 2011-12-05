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

void
initrd_init_effect (void*,
		    int&);

void
initrd_schedule ();

void
initrd_finish (bool,
	       void*);

struct initrd_init_tag { };
typedef input_action<initrd_init_tag, void*, int, initrd_init_effect, initrd_schedule, initrd_finish> initrd_init_type;
initrd_init_type initrd_init;

#endif /* __initrd_automaton_hpp__ */

