#ifndef __welcome_message_hpp__
#define __welcome_message_hpp__

/*
  File
  ----
  welcome_message.hpp
  
  Description
  -----------
  An object that prints a welcome message when constructed.

  Authors:
  Justin R. Wilson
*/

#include "kput.hpp"

class welcome_message {
public:
  welcome_message ()
  {
    // Print a welcome message.
    clear_console ();
    kputs ("Lily\n");
  }
};

#endif /* __welcome_message_hpp__ */
