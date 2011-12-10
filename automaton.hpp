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
#include "action_traits.hpp"
#include "static_assert.hpp"
#include <type_traits>
#include "global_descriptor_table.hpp"

class automaton {
public:
  struct action {
    action_type_t type;
    const void* action_entry_point;
    parameter_mode_t parameter_mode;
    size_t value_size;

    action (action_type_t t,
	    const void* aep,
	    parameter_mode_t pm,
	    size_t vs) :
      type (t),
      action_entry_point (aep),
      parameter_mode (pm),
      value_size (vs)
    { }
  };

private:
  // Automaton identifier.
  aid_t aid_;
  // Segments including privilege.
  uint32_t code_segment_;
  uint32_t stack_segment_;
  // Physical address of the automaton's page directory.
  physical_address_t const page_directory_;
  // Table of action descriptors for guiding execution, checking bindings, etc.
  typedef std::unordered_map<const void*, action, std::hash<const void*>, std::equal_to<const void*>, system_allocator<std::pair<const void* const, action> > > action_map_type;
  action_map_type action_map_;
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef std::vector<vm_area_interface*, system_allocator<vm_area_interface*> > memory_map_type;
  memory_map_type memory_map_;
  // Heap area.
  vm_heap_area* heap_area_;
  // Stack area.
  vm_stack_area* stack_area_;

public:
  class const_action_iterator : public std::iterator<std::bidirectional_iterator_tag, action> {
  private:
    action_map_type::const_iterator pos_;
    
  public:
    const_action_iterator (const action_map_type::const_iterator& p) :
      pos_ (p)
    { }

    bool
    operator== (const const_action_iterator& other) const
    {
      return pos_ == other.pos_;
    }

    const action*
    operator-> () const
    {
      return &(pos_->second);
    }
  };

private:

  static bool
  intersect (const vm_area_interface* x,
	     const vm_area_interface* y)
  {
    return !(x->end () <= y->begin () || y->end () <= x->begin ());
  }

  struct compare_vm_area {
    bool
    operator () (const vm_area_interface* const x,
		 const vm_area_interface* const y) const
    {
      return x->begin () < y->begin ();
    }
  };

  struct contains_address {
    const void* address;

    contains_address (const void* a) :
      address (a)
    { }

    bool
    operator () (const vm_area_interface* const x) const
    {
      return x->begin () <= address && address < x->end ();
    }
  };

  memory_map_type::const_iterator
  find_by_address (const vm_area_interface* area) const
  {
    return std::lower_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
  }

  memory_map_type::iterator
  find_by_address (const vm_area_interface* area)
  {
    return std::lower_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
  }

  template <class Action>
  void
  add_action_ (const void* action_entry_point)
  {
    action ac (Action::action_type, action_entry_point, Action::parameter_mode, Action::value_size);
    std::pair<typename action_map_type::iterator, bool> r = action_map_.insert (std::make_pair (ac.action_entry_point, ac));
    kassert (r.second);
  }

public:

  automaton (aid_t aid,
	     descriptor_constants::privilege_t privilege,
	     physical_address_t page_directory) :
    aid_ (aid),
    page_directory_ (page_directory),
    heap_area_ (0),
    stack_area_ (0)
  {
    kassert (is_aligned (page_directory, PAGE_SIZE));

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
  }

  bool
  address_in_use (const void* address) const
  {
    if (address < PAGING_AREA) {
      // TODO: Use binary search.
      return std::find_if (memory_map_.begin (), memory_map_.end (), contains_address (address)) != memory_map_.end ();
    }
    else {
      return true;
    }
  }

  aid_t
  aid () const
  {
    return aid_;
  }

  const void*
  stack_pointer () const
  {
    kassert (stack_area_ != 0);
    return stack_area_->end ();
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

  // TODO:  Return an iterator.
  bool
  insert_vm_area (vm_area_interface* area)
  {
    kassert (area != 0);
    
    // Find the location to insert.
    memory_map_type::iterator pos = find_by_address (area);

    // Ensure that the areas don't conflict.
    if (pos != memory_map_.begin ()) {
      memory_map_type::const_iterator prev = pos - 1;
      if (intersect (*prev, area)) {
	return false;
      }
    }

    if (pos != memory_map_.end ()) {
      if (intersect (area, *pos)) {
	return false;
      }
    }

    memory_map_.insert (pos, area);
    return true;
  }
  
  bool
  insert_heap_area (vm_heap_area* area)
  {
    if (insert_vm_area (area)) {
      heap_area_ = area;
      return true;
    }
    else {
      return false;
    }
  }

  bool
  insert_stack_area (vm_stack_area* area)
  {
    if (insert_vm_area (area)) {
      stack_area_ = area;
      return true;
    }
    else {
      return false;
    }
  }

  void*
  allocate (size_t size)
  {
    kassert (heap_area_ != 0);

    if (size != 0) {
      void* const old_end = const_cast<void*> (heap_area_->end ());
      const void* const new_end = reinterpret_cast<uint8_t*> (old_end) + size;
      // Find the heap.
      memory_map_type::const_iterator pos = std::find (memory_map_.begin (), memory_map_.end (), heap_area_);
      kassert (pos != memory_map_.end ());
      // Move to the next.
      ++pos;
      if (new_end <= (*pos)->begin ()) {
	// The allocation does not interfere with next area.  Success.
	heap_area_->end (new_end);
	return old_end;
      }
      else {
	// Failure.
	return 0;
      }
    }
    else {
      // Return the current end of the heap.
      return const_cast<void*> (heap_area_->end ());
    }
  }

  void*
  reserve (size_t size)
  {
    kassert (is_aligned (size, PAGE_SIZE));
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);
    kassert (heap_area_->end () <= stack_area_->begin ());

    // Find the stack.
    memory_map_type::reverse_iterator high = std::find (memory_map_.rbegin (), memory_map_.rend (), stack_area_);
    kassert (high != memory_map_.rend ());

    // Find the area before the stack.
    memory_map_type::reverse_iterator low = high + 1;
    kassert (low != memory_map_.rend ());

    do {
      // Try to insert between the areas.
      if (reinterpret_cast<size_t> ((*high)->begin ()) - reinterpret_cast<size_t> ((*low)->end ()) >= size) {
	void* begin = const_cast<uint8_t*> (reinterpret_cast<const uint8_t*> ((*high)->begin ()) - size);
	memory_map_.insert (low.base (), new (system_alloc ()) vm_reserved_area (begin, (*high)->begin ()));
	return begin;
      }
      else {
	// Try again.
	++high;
	++low;
      }
    } while ((*low) != heap_area_);

    // No space.
    return 0;
  }

  void
  unreserve (const void* address)
  {
    // TODO: Use binary search.
    memory_map_type::iterator pos = std::find_if (memory_map_.begin (), memory_map_.end (), contains_address (address));
    kassert (pos != memory_map_.end ());

    vm_area_interface* area = *pos;
    memory_map_.erase (pos);
    destroy (area, system_alloc ());
  }

  bool
  verify_span (const void* ptr,
	       size_t size) const
  {
    kassert (0);
    // vm_reserved_area k (ptr, static_cast<const uint8_t*> (ptr) + size);
    // typename memory_map_type::const_iterator pos = find_by_address (&k);
    // kassert (pos != memory_map_.end ());
    // return (*pos)->is_data_area ();
  }
  
  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs)
  {
    kassert (0);
    // vm_reserved_area k (address, static_cast<const uint8_t*> (address) + 1);
    // typename memory_map_type::const_iterator pos = find_by_address (&k);
    // kassert (pos != memory_map_.end ());
    // (*pos)->page_fault (address, error, regs);
  }

  template <class Action>
  void
  add_action (void (*action_entry_point) (void))
  {
    STATIC_ASSERT (is_action<Action>::value);
    STATIC_ASSERT ((Action::action_type == INPUT && Action::parameter_mode == NO_PARAMETER && Action::value_size == 0) ||
		   (Action::action_type == OUTPUT && Action::parameter_mode == NO_PARAMETER) ||
		   (Action::action_type == INTERNAL && Action::parameter_mode == NO_PARAMETER));
    
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (no_parameter_tag,
			void (*action_entry_point) (typename Action::value_type))
  {
    STATIC_ASSERT (is_input_action<Action>::value);
    STATIC_ASSERT (Action::parameter_mode == NO_PARAMETER && Action::value_size != 0);
    
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (parameter_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    STATIC_ASSERT (is_action<Action>::value);
    STATIC_ASSERT ((Action::action_type == INPUT && Action::parameter_mode == PARAMETER && Action::value_size == 0) ||
  		   (Action::action_type == OUTPUT && Action::parameter_mode == PARAMETER) ||
  		   (Action::action_type == INTERNAL && Action::parameter_mode == PARAMETER));

    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (auto_parameter_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    STATIC_ASSERT (is_action<Action>::value);
    STATIC_ASSERT ((Action::action_type == INPUT && Action::parameter_mode == AUTO_PARAMETER && Action::value_size == 0) ||
  		   (Action::action_type == OUTPUT && Action::parameter_mode == AUTO_PARAMETER));
    
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action,
	    class T>
  void
  add_action (void (*action_entry_point) (T))
  {
    add_action_dispatch_<Action> (typename Action::parameter_category (), action_entry_point);
  }

  template <class Action>
  void
  add_action (void (*action_entry_point) (typename Action::parameter_type, typename Action::value_type))
  {
    STATIC_ASSERT (is_input_action<Action>::value);
    STATIC_ASSERT ((Action::parameter_mode == PARAMETER || Action::parameter_mode == AUTO_PARAMETER) && Action::value_size != 0);

    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  const_action_iterator
  action_end () const
  {
    return const_action_iterator (action_map_.end ());
  }

  const_action_iterator
  action_find (const void* action_entry_point) const
  {
    return const_action_iterator (action_map_.find (action_entry_point));
  }

};

#endif /* __automaton_hpp__ */
