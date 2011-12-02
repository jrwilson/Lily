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
#include "automaton.hpp"

class binding_manager {
public:
  typedef std::unordered_set<input_action, std::hash<input_action>, std::equal_to<input_action>, list_allocator<input_action> > input_action_set_type;

private:
  list_alloc& alloc_;
  typedef std::unordered_map<output_action, input_action_set_type, std::hash<output_action>, std::equal_to<output_action>, list_allocator<std::pair<const output_action, input_action_set_type> > > bindings_type;
  bindings_type bindings_;

  void
  dump_bindings () const
  {
    for (bindings_type::const_iterator pos1 = bindings_.begin ();
	 pos1 != bindings_.end ();
	 ++pos1) {
      kputs ("output_automaton = "); kputp (pos1->first.automaton); kputs ("\n");
      kputs ("output_action_entry_point = "); kputp (pos1->first.action_entry_point); kputs ("\n");
      kputs ("output_parameter = "); kputx32 (pos1->first.parameter); kputs ("\n");

      for (input_action_set_type::const_iterator pos2 = pos1->second.begin ();
      	   pos2 != pos1->second.end ();
      	   ++pos2) {
      	kputs ("input_automaton = "); kputp (pos2->automaton); kputs ("\n");
      	kputs ("input_action_entry_point = "); kputp (pos2->action_entry_point); kputs ("\n");
      	kputs ("input_parameter = "); kputx32 (pos2->parameter); kputs ("\n");
      }
    }
  }

public:
  binding_manager (list_alloc& a) :
    alloc_ (a),
    bindings_ (3, bindings_type::hasher (), bindings_type::key_equal (), bindings_type::allocator_type (a))
  { }

  void
  bind (automaton* output_automaton,
	void* output_action_entry_point,
	parameter_t output_parameter,
	automaton* input_automaton,
	void* input_action_entry_point,
	parameter_t input_parameter)
  {
    kassert (output_automaton != 0);
    kassert (output_automaton->get_action_type (output_action_entry_point) == OUTPUT);
    kassert (input_automaton != 0);
    kassert (input_automaton->get_action_type (input_action_entry_point) == INPUT);
    
    /* TODO:  All of the bind checks. */
    output_action oa (output_automaton, output_action_entry_point, output_parameter);
    input_action ia (input_automaton, input_action_entry_point, input_parameter);

    std::pair<bindings_type::iterator, bool> r = bindings_.insert (std::make_pair (oa, input_action_set_type (3, input_action_set_type::hasher (), input_action_set_type::key_equal (), input_action_set_type::allocator_type (alloc_))));
    r.first->second.insert (ia);
    dump_bindings ();
  }
  
  const binding_manager::input_action_set_type*
  get_bound_inputs (automaton* output_automaton,
		    void* output_action_entry_point,
		    parameter_t output_parameter)
  {
    output_action oa (output_automaton, output_action_entry_point, output_parameter);
    bindings_type::const_iterator pos = bindings_.find (oa);
    if (pos != bindings_.end ()) {
      return &pos->second;
    }
    else {
      return 0;
    }
  }
  
};

#endif /* __binding_manager_hpp__ */
