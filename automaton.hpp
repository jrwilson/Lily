#ifndef __automaton_hpp__
#define __automaton_hpp__

/*
  File
  ----
  automaton.hpp
  
  Description
  -----------
  The automaton data structure.

  Authors:
  Justin R. Wilson
*/

#include "syscall_def.hpp"
#include "vm_area.hpp"
#include <unordered_map>
#include <vector>
#include "list_allocator.hpp"
#include "descriptor.hpp"
#include "action_macros.hpp"

class automaton {
public:
  enum action_type {
    INPUT,
    OUTPUT,
    INTERNAL,
    NO_ACTION,
  };

  struct action {
    action_type type;
    union {
      input_func input;
      output_func output;
      internal_func internal;
    };
    size_t message_size;

    action () :
      type (NO_ACTION),
      input (0),
      message_size (0)
    { }
  };

private:
  struct compare_vm_area {
    bool
    operator () (const vm_area_base* const x,
		 const vm_area_base* const y) const
    {
      return x->begin () < y->begin ();
    }
  };

  list_alloc& alloc_;
  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  typedef std::unordered_map<void*, action, std::hash<void*>, std::equal_to<void*>, list_allocator<std::pair<void* const, action> > > action_map_type;
  action_map_type action_map_;
  physical_address page_directory_;
  /* Stack pointer (constant). */
  logical_address stack_pointer_;
  /* Memory map. Consider using a set/map if insert/remove becomes too expensive. */
  typedef std::vector<vm_area_base*, list_allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  /* Default privilege for new VM_AREA_DATA. */
  paging_constants::page_privilege_t page_privilege_;

  memory_map_type::const_iterator
  find_by_address (const vm_area_base* area) const;

  memory_map_type::iterator
  find_by_address (const vm_area_base* area);

  memory_map_type::iterator
  find_by_size (size_t size);

  memory_map_type::iterator
  merge (memory_map_type::iterator pos);

  void
  insert_into_free_area (memory_map_type::iterator pos,
			 vm_area_base* area);

  template <class InputAction>
  void
  add_action_ (const InputAction&,
	       void (*ptr) (typename InputAction::parameter_type, typename InputAction::value_type&),
	       input_action_tag)
  {
    action ac;
    ac.type = INPUT;
    ac.input = reinterpret_cast<input_func> (ptr);
    ac.message_size = sizeof (typename InputAction::value_type);
    kassert (action_map_.insert (std::make_pair (reinterpret_cast<void*> (ptr), ac)).second);
  }
  
  
  template <class OutputAction>
  void
  add_action_ (const OutputAction&,
	       void (*ptr) (typename OutputAction::parameter_type),
	       output_action_tag)
  {
    action ac;
    ac.type = OUTPUT;
    ac.output = reinterpret_cast<output_func> (ptr);
    ac.message_size = sizeof (typename OutputAction::value_type);
    kassert (action_map_.insert (std::make_pair (reinterpret_cast<void*> (ptr), ac)).second);
  }
  
  template <class InternalAction>
  void
  add_action_ (const InternalAction&,
	       void (*ptr) (typename InternalAction::parameter_type),
	       internal_action_tag)
  {
    action ac;
    ac.type = INTERNAL;
    ac.internal = ptr;
    ac.message_size = 0;
    kassert (action_map_.insert (std::make_pair (reinterpret_cast<void*> (ptr), ac)).second);
  }

public:
  automaton (list_alloc& a,
	     descriptor_constants::privilege_t privilege,
	     physical_address page_directory,
	     logical_address stack_pointer,
	     logical_address memory_ceiling,
	     paging_constants::page_privilege_t page_privilege);

  logical_address
  stack_pointer () const;
  
  physical_address
  page_directory () const;

  uint32_t
  code_segment () const;

  uint32_t
  stack_segment () const;

  template <class T>
  bool
  insert_vm_area (const T& area)
  {
    kassert (area.end () <= memory_map_.back ()->begin ());
    
    // Find the location to insert.
    memory_map_type::iterator pos = find_by_address (&area);
    
    if ((*pos)->type () == VM_AREA_FREE && area.size () < (*pos)->size ()) {
      insert_into_free_area (pos, new (alloc_) T (area));
      return true;
    }
    else {
      return false;
    }
  }
  
  logical_address
  alloc (size_t size) __attribute__((warn_unused_result));

  logical_address
  reserve (size_t size);
  
  void
  unreserve (logical_address address);

  bool
  verify_span (void* ptr,
	       size_t size) const;
  
  void
  page_fault (logical_address address,
	      uint32_t error,
	      registers* regs);
  
  template <class Action>
  void
  add_action (const Action& ac)
  {
    add_action_ (ac, &Action::action, typename Action::action_category ());
  }

  action
  get_action (local_func) const;
};

#endif /* __automaton_hpp__ */
