/*
  File
  ----
  boot_automaton.cpp
  
  Description
  -----------
  Special automaton for booting.

  Authors:
  Justin R. Wilson
*/

#include "boot_automaton.hpp"
#include "kassert.hpp"
#include "string.hpp"
#include "page_fault_handler.hpp"
#include <utility>

using namespace std::rel_ops;

