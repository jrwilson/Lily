#ifndef __binding_manager_hpp__
#define __binding_manager_hpp__

/*
  File
  ----
  binding_manager.hpp
  
  Description
  -----------
  Manage bindings between output actions and input actions.

  Authors:
  Justin R. Wilson
*/

#include <unordered_set>
#include <unordered_map>
#include "automaton_interface.hpp"

struct action {
  automaton_interface* automaton;
  void* action_entry_point;
  parameter_t parameter;

  action () :
    automaton (0),
    action_entry_point (0),
    parameter (0)
  { }

  action (automaton_interface* a,
	  void* aep,
	  parameter_t p) :
    automaton (a),
    action_entry_point (aep),
    parameter (p)
  { }
  
  bool
  operator== (const action& other) const
  {
    return automaton == other.automaton && action_entry_point == other.action_entry_point && parameter == other.parameter;
  }
};

struct input_action : public action {
  input_action (automaton_interface* a,
		void* aep,
		parameter_t p) :
    action (a, aep, p)
  { }
};

struct local_action : public action {
  local_action ()
  { }

  local_action (automaton_interface* a,
		void* aep,
		parameter_t p) :
    action (a, aep, p)
  { }
};

struct output_action : public local_action {
  output_action (automaton_interface* a,
		 void* aep,
		 parameter_t p) :
    local_action (a, aep, p)
  { }
};

struct internal_action : public local_action {
  internal_action (automaton_interface* a,
		   void* aep,
		   parameter_t p) :
    local_action (a, aep, p)
  { }
};

template <typename AllocatorTag, template <typename> class Allocator>
class binding_manager {
public:
  typedef std::unordered_set<input_action, std::hash<input_action>, std::equal_to<input_action>, Allocator<input_action> > input_action_set_type;

private:
  typedef std::unordered_map<output_action, input_action_set_type, std::hash<output_action>, std::equal_to<output_action>, Allocator<std::pair<const output_action, input_action_set_type> > > bindings_type;
  static bindings_type* bindings_;

public:
  static void
  initialize ()
  {
    kassert (bindings_ == 0);
    bindings_ = new (AllocatorTag ()) bindings_type ();
  }

  static void
  bind (automaton_interface* output_automaton,
	void* output_action_entry_point,
	parameter_t output_parameter,
	automaton_interface* input_automaton,
	void* input_action_entry_point,
	parameter_t input_parameter)
  {
    kassert (bindings_ != 0);
    kassert (output_automaton != 0);
    kassert (output_automaton->get_action_type (output_action_entry_point) == OUTPUT);
    kassert (input_automaton != 0);
    kassert (input_automaton->get_action_type (input_action_entry_point) == INPUT);
    
    /* TODO:  All of the bind checks. */
    
    output_action oa (output_automaton, output_action_entry_point, output_parameter);
    input_action ia (input_automaton, input_action_entry_point, input_parameter);
    
    std::pair<typename bindings_type::iterator, bool> r = bindings_->insert (std::make_pair (oa, input_action_set_type ()));
    r.first->second.insert (ia);
  }
  
  static const binding_manager::input_action_set_type*
  get_bound_inputs (automaton_interface* output_automaton,
		    void* output_action_entry_point,
		    parameter_t output_parameter)
  {
    kassert (bindings_ != 0);
    
    output_action oa (output_automaton, output_action_entry_point, output_parameter);
    typename bindings_type::const_iterator pos = bindings_->find (oa);
    if (pos != bindings_->end ()) {
      return &pos->second;
    }
    else {
      return 0;
    }
  }
  
};

#endif /* __binding_manager_hpp__ */
