/*
  File
  ----
  aid_manager.cpp
  
  Description
  -----------
  A class for managing the set of automaton identifiers (aids).

  Authors:
  Justin R. Wilson
*/

#include "aid_manager.hpp"

aid_t aid_manager::current_;
aid_manager::aid_set_type aid_manager::aid_set_;
