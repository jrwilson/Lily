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
#include "descriptor.hpp"
#include "action_macros.hpp"
#include "static_assert.hpp"
#include <type_traits>
#include "global_descriptor_table.hpp"
#include "automaton_interface.hpp"

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

template <class Alloc, template <typename> class Allocator>
class automaton : public automaton_interface {
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

  Alloc& alloc_;
  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  typedef std::unordered_map<size_t, action, std::hash<size_t>, std::equal_to<size_t>, Allocator<std::pair<size_t const, action> > > action_map_type;
  action_map_type action_map_;
  physical_address_t const page_directory_;
  /* Stack pointer (constant). */
  const void* const stack_pointer_;
  /* Memory map. Consider using a set/map if insert/remove becomes too expensive. */
  // TODO:  Change to set so all allocations are fixed size.
  typedef std::vector<vm_area_base*, Allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  /* Default privilege for new VM_AREA_DATA. */
  paging_constants::page_privilege_t const page_privilege_;

  typename memory_map_type::const_iterator
  find_by_address (const vm_area_base* area) const
  {
    // Find the location to insert.
    typename memory_map_type::const_iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    kassert (pos != memory_map_.begin ());
    return --pos;
  }

  typename memory_map_type::iterator
  find_by_address (const vm_area_base* area)
  {
    // Find the location to insert.
    typename memory_map_type::iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    kassert (pos != memory_map_.begin ());
    return --pos;
  }

  typename memory_map_type::iterator
  find_by_size (size_t size)
  {
    typename memory_map_type::iterator pos;
    for (pos = memory_map_.begin ();
	 pos != memory_map_.end ();
	 ++pos) {
      if ((*pos)->type () == VM_AREA_FREE && size <= (*pos)->size ()) {
	break;
      }
    }
    
    return pos;
  }

  typename memory_map_type::iterator
  merge (typename memory_map_type::iterator pos)
  {
    if (pos != memory_map_.end ()) {
      typename memory_map_type::iterator next (pos);
      ++next;
      if (next != memory_map_.end ()) {
	vm_area_base* temp = *next;
	if ((*pos)->merge (*temp)) {
	  memory_map_.erase (next);
	  destroy (temp, alloc_);
	}
      }
    }
    
    return pos;
  }

  void
  insert_into_free_area (typename memory_map_type::iterator pos,
			 vm_area_base* area)
  {
    kassert ((*pos)->type () == VM_AREA_FREE);
    kassert (area->size () < (*pos)->size ());
    
    const void* left_begin = (*pos)->begin ();
    const void* left_end = area->begin ();
    
    const void* right_begin = area->end ();
    const void* right_end = (*pos)->end ();
    
    // Take out the old entry.
    destroy (*pos, alloc_);
    pos = memory_map_.erase (pos);
    
    // Split the area.
    if (right_begin < right_end) {
      pos = merge (memory_map_.insert (pos, new (alloc_) vm_free_area (right_begin, right_end)));
    }
    pos = merge (memory_map_.insert (pos, area));
    if (left_begin < left_end) {
      pos = merge (memory_map_.insert (pos, new (alloc_) vm_free_area (left_begin, left_end)));
    }
    
    if (pos != memory_map_.begin ()) {
      merge (--pos);
    }
    
  }

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

  automaton (Alloc& a,
	     descriptor_constants::privilege_t privilege,
	     physical_address_t page_directory,
	     const void* stack_pointer,
	     const void* memory_ceiling,
	     paging_constants::page_privilege_t page_privilege) :
    alloc_ (a),
    action_map_ (3, typename action_map_type::hasher (), typename action_map_type::key_equal (), typename action_map_type::allocator_type (alloc_)),
    page_directory_ (page_directory),
    stack_pointer_ (stack_pointer),
    memory_map_ (typename memory_map_type::allocator_type (alloc_)),
    page_privilege_ (page_privilege)
  {
    switch (privilege) {
    case descriptor_constants::RING0:
      code_segment_ = KERNEL_CODE_SELECTOR | descriptor_constants::RING0;
      stack_segment_ = KERNEL_DATA_SELECTOR | descriptor_constants::RING0;
      break;
    case descriptor_constants::RING1:
    case descriptor_constants::RING2:
      /* These rings are not supported. */
      kassert (0);
      break;
    case descriptor_constants::RING3:
      /* Not supported yet. */
      kassert (0);
      break;
    }
    
    memory_ceiling = align_up (memory_ceiling, PAGE_SIZE);
    
    memory_map_.push_back (new (alloc_) vm_free_area (0, memory_ceiling));
    memory_map_.push_back (new (alloc_) vm_reserved_area (memory_ceiling, 0));
  }

  const void*
  stack_pointer () const
  {
    return stack_pointer_;
  }
  
  physical_address_t
  page_directory () const
  {
    return page_directory_;
  }

  uint32_t
  code_segment () const
  {
    return code_segment_;
  }


  uint32_t
  stack_segment () const
  {
    return stack_segment_;
  }

  bool
  insert_vm_area (vm_area_base* area)
  {
    kassert (area != 0);
    kassert (area->end () <= memory_map_.back ()->begin ());
    
    // Find the location to insert.
    typename memory_map_type::iterator pos = find_by_address (area);
    
    if ((*pos)->type () == VM_AREA_FREE && area->size () < (*pos)->size ()) {
      insert_into_free_area (pos, area);
      return true;
    }
    else {
      return false;
    }
  }
  
  void
  remove_vm_area (vm_area_base* area)
  {
    kassert (area != 0);
    
    typename memory_map_type::iterator pos = std::find (memory_map_.begin (), memory_map_.end (), area);
    kassert (pos != memory_map_.end ());
    memory_map_.erase (pos);
    destroy (area, alloc_);
  }

  void*
  alloc (size_t size) __attribute__((warn_unused_result))
  {
    kassert (size > 0);
    
    typename memory_map_type::iterator pos = find_by_size (size);
    if (pos != memory_map_.end ()) {
      void* retval = const_cast<void*> ((*pos)->begin ());
      insert_into_free_area (pos, new (alloc_) vm_data_area (retval, static_cast<uint8_t*> (retval) + size, page_privilege_));
      return retval;
    }
    else {
      return 0;
    }
  }

  void*
  reserve (size_t size)
  {
    kassert (size > 0);
    
    typename memory_map_type::iterator pos = find_by_size (size);
    
    if (pos != memory_map_.end ()) {
      void* retval = const_cast<void*> ((*pos)->begin ());
      insert_into_free_area (pos, new (alloc_) vm_reserved_area (retval, static_cast<uint8_t*> (retval) + size));
      return retval;
    }
    else {
      return 0;
    }
  }
  
  void
  unreserve (const void* address)
  {
    kassert (address != 0);
    kassert (is_aligned (address, PAGE_SIZE));
    
    vm_reserved_area k (address, static_cast<const uint8_t*> (address) + PAGE_SIZE);
    
    typename memory_map_type::iterator pos = find_by_address (&k);
    kassert (pos != memory_map_.end ());
    kassert ((*pos)->begin () == address);
    kassert ((*pos)->type () == VM_AREA_RESERVED);
    
    vm_free_area* f = new (alloc_) vm_free_area ((*pos)->begin (), (*pos)->end ());
    
    destroy (*pos, alloc_);
    pos = memory_map_.insert (memory_map_.erase (pos), f);
    
    merge (pos);
    if (pos != memory_map_.begin ()) {
      merge (--pos);
    }
  }

  bool
  verify_span (const void* ptr,
	       size_t size) const
  {
    vm_reserved_area k (ptr, static_cast<const uint8_t*> (ptr) + size);
    typename memory_map_type::const_iterator pos = find_by_address (&k);
    kassert (pos != memory_map_.end ());
    return (*pos)->is_data_area ();
  }
  
  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs)
  {
    vm_reserved_area k (address, static_cast<const uint8_t*> (address) + 1);
    typename memory_map_type::const_iterator pos = find_by_address (&k);
    kassert (pos != memory_map_.end ());
    (*pos)->page_fault (address, error, regs);
  }
  
  template <class ActionTraits>
  void
  add_action ()
  {
    add_action_<ActionTraits> (typename ActionTraits::action_category ());
  }

  action
  get_action (size_t action_entry_point) const
  {
    typename action_map_type::const_iterator pos = action_map_.find (action_entry_point);
    if (pos != action_map_.end ()) {
      return pos->second;
    }
    else {
      return action ();
    }
  }

};

#endif /* __automaton_hpp__ */
