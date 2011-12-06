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
#include "static_assert.hpp"
#include <type_traits>

// Size of the temporary buffer used to store the values produced by output actions.
const size_t MESSAGE_BUFFER_SIZE = 512;

template <class Category, bool Check>
struct input_action_helper : public std::false_type { };

template <>
struct input_action_helper<input_action_tag, true> : public std::true_type { };

template <class T>
struct is_input_action : public input_action_helper<typename T::action_category, sizeof (typename T::parameter_type) == sizeof (void*) && sizeof (typename T::message_type) <= MESSAGE_BUFFER_SIZE> { };

template <class Category, bool Check>
struct output_action_helper : public std::false_type { };

template <>
struct output_action_helper<output_action_tag, true> : public std::true_type { };

template <class T>
struct is_output_action : public output_action_helper<typename T::action_category, sizeof (typename T::parameter_type) == sizeof (void*) && sizeof (typename T::message_type) <= MESSAGE_BUFFER_SIZE> { };

template <class Category, bool Check>
struct internal_action_helper : public std::false_type { };

template <>
struct internal_action_helper<internal_action_tag, true> : public std::true_type { };

template <class T>
struct is_internal_action : public internal_action_helper<typename T::action_category, sizeof (typename T::parameter_type) == sizeof (void*)> { };

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
    size_t action_entry_point;
    size_t message_size;
    bool is_parameterized;

    action () :
      type (NO_ACTION),
      action_entry_point (0),
      message_size (0),
      is_parameterized (false)
    { }

    action (const action& other) :
      type (other.type),
      action_entry_point (other.action_entry_point),
      message_size (other.message_size),
      is_parameterized (other.is_parameterized)
    { }

    action&
    operator= (const action& other)
    {
      if (this != &other) {
	type = other.type;
	action_entry_point = other.action_entry_point;
	message_size = other.message_size;
	is_parameterized = other.is_parameterized;
      }
      return *this;
    }
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
  typedef std::unordered_map<size_t, action, std::hash<size_t>, std::equal_to<size_t>, list_allocator<std::pair<size_t const, action> > > action_map_type;
  action_map_type action_map_;
  physical_address_t const page_directory_;
  /* Stack pointer (constant). */
  const void* const stack_pointer_;
  /* Memory map. Consider using a set/map if insert/remove becomes too expensive. */
  typedef std::vector<vm_area_base*, list_allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  /* Default privilege for new VM_AREA_DATA. */
  paging_constants::page_privilege_t const page_privilege_;

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

  template <class ActionTraits>
  void
  add_action_ (input_action_tag)
  {
    STATIC_ASSERT (is_input_action<ActionTraits>::value);
    action ac;
    ac.type = INPUT;
    ac.action_entry_point = ActionTraits::action_entry_point;
    ac.message_size = sizeof (typename ActionTraits::message_type);
    ac.is_parameterized = true;
    kassert (action_map_.insert (std::make_pair (ac.action_entry_point, ac)).second);
  }

  template <class ActionTraits>
  void
  add_action_ (output_action_tag)
  {
    STATIC_ASSERT (is_output_action<ActionTraits>::value);
    action ac;
    ac.type = OUTPUT;
    ac.action_entry_point = ActionTraits::action_entry_point;
    ac.message_size = sizeof (typename ActionTraits::message_type);
    ac.is_parameterized = true;
    kassert (action_map_.insert (std::make_pair (ac.action_entry_point, ac)).second);
  }

  template <class ActionTraits>
  void
  add_action_ (internal_action_tag)
  {
    STATIC_ASSERT (is_internal_action<ActionTraits>::value);
    action ac;
    ac.type = INTERNAL;
    ac.action_entry_point = ActionTraits::action_entry_point;
    ac.message_size = 0;
    ac.is_parameterized = true;
    kassert (action_map_.insert (std::make_pair (ac.action_entry_point, ac)).second);
  }

public:
  automaton (list_alloc& a,
	     descriptor_constants::privilege_t privilege,
	     physical_address_t page_directory,
	     const void* stack_pointer,
	     const void* memory_ceiling,
	     paging_constants::page_privilege_t page_privilege);

  const void*
  stack_pointer () const;
  
  physical_address_t
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
  
  void*
  alloc (size_t size) __attribute__((warn_unused_result));

  void*
  reserve (size_t size);
  
  void
  unreserve (const void* address);

  bool
  verify_span (const void* ptr,
	       size_t size) const;
  
  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs);
  
  template <class ActionTraits>
  void
  add_action ()
  {
    add_action_<ActionTraits> (typename ActionTraits::action_category ());
  }

  action
  get_action (size_t action_entry_point) const;
};

#endif /* __automaton_hpp__ */
