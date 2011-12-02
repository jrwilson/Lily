#ifndef __welcome_message_hpp__
#define __welcome_message_hpp__

/*
  File
  ----
  welcome_message.hpp
  
  Description
  -----------
  An object that prints a welcome message.

  Authors:
  Justin R. Wilson
*/

#include "kput.hpp"

struct welcome_message {
  welcome_message ()
  {
    // Print a welcome message.
    clear_console ();
    kputs ("Lily\n");
  }
};

#endif /* __welcome_message_hpp__ */
