#ifndef __automaton_interface_hpp__
#define __automaton_interface_hpp__

/*
  File
  ----
  automaton_interface.hpp
  
  Description
  -----------
  The automaton interface.  Mainly used for memory mapping issues.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include "vm_area.hpp"

class automaton_interface {
public:
  virtual void*
  alloc (size_t) = 0;

  virtual bool
  insert_vm_area (vm_area_base* area) = 0;

  virtual void
  remove_vm_area (vm_area_base* area) = 0;
};

#endif /* __automaton_interface_hpp__ */
