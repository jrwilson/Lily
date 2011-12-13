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

  struct compare_vm_area {
    bool
    operator () (const vm_area_interface* const x,
		 const vm_area_interface* const y) const
    {
      return x->begin () < y->begin ();
    }
  };

  memory_map_type::const_iterator
  find_address (const void* address) const
  {
    vm_reserved_area k (address, address);
    memory_map_type::const_iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), &k, compare_vm_area ());
    // We know that area->begin () < (*pos)->begin ().

    if (pos != memory_map_.begin ()) {
      --pos;
      if (address < (*pos)->end ()) {
	return pos;
      }
    }
    
    return memory_map_.end ();
  }

  memory_map_type::iterator
  find_address (const void* address)
  {
    vm_reserved_area k (address, address);
    memory_map_type::iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), &k, compare_vm_area ());
    // We know that area->begin () < (*pos)->begin ().

    if (pos != memory_map_.begin ()) {
      --pos;
      if (address < (*pos)->end ()) {
	return pos;
      }
    }
    
    return memory_map_.end ();
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
      return find_address (address) != memory_map_.end ();
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
    memory_map_type::iterator pos = std::upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    // We know that area->begin () < (*pos)->begin ().

    // Ensure that the areas don't conflict.
    if (pos != memory_map_.begin ()) {
      memory_map_type::const_iterator prev = pos - 1;
      if (area->begin () < (*prev)->end ()) {
	return false;
      }
    }

    if (pos != memory_map_.end ()) {
      if ((*pos)->begin () < area->end ()) {
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
  sbrk (ptrdiff_t size)
  {
    kassert (heap_area_ != 0);

    void* const old_end = const_cast<void*> (heap_area_->end ());
    const void* const new_end = reinterpret_cast<uint8_t*> (old_end) + size;
    if (new_end < old_end) {
      // Shrunk too much.
      // Fail.
      return reinterpret_cast<void*> (-1);
    }
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
      return reinterpret_cast<void*> (-1);
    }
  }

  bool
  verify_span (const void* ptr,
	       size_t size) const
  {
    memory_map_type::const_iterator pos = find_address (ptr);
    return pos != memory_map_.end () && (*pos)->begin () <= ptr && (static_cast<const uint8_t*> (ptr) + size) <= (*pos)->end ();
  }
  
  void
  page_fault (const void* address,
	      uint32_t error,
	      registers* regs)
  {
    memory_map_type::const_iterator pos = find_address (address);
    if (pos != memory_map_.end ()) {
      (*pos)->page_fault (address, error, regs);
    }
    else {
      kout << "Page Fault" << endl;
      kout << "aid = " << aid_ << " address = " << hexformat (address) << " error = " << hexformat (error) << endl;
      kout << *regs << endl;
      // TODO:  Handle page fault.
      kassert (0);
    }
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
