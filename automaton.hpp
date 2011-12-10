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

  aid_t aid_;
  /* Segments including privilege. */
  uint32_t code_segment_;
  uint32_t stack_segment_;

  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  typedef std::unordered_map<const void*, action, std::hash<const void*>, std::equal_to<const void*>, system_allocator<std::pair<const void* const, action> > > action_map_type;
  action_map_type action_map_;
  // physical_address_t const page_directory_;
  // /* Stack pointer (constant). */
  // const void* const stack_pointer_;
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef std::vector<vm_area_interface*, system_allocator<vm_area_interface*> > memory_map_type;
  memory_map_type memory_map_;
  // /* Default privilege for new VM_AREA_DATA. */
  // paging_constants::page_privilege_t const page_privilege_;

public:
  class const_action_iterator : public std::iterator<std::bidirectional_iterator_tag, action> {
  private:
    typename action_map_type::const_iterator pos_;
    
  public:
    const_action_iterator (const typename action_map_type::const_iterator& p) :
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

  typename memory_map_type::const_iterator
  find_by_address (const vm_area_interface* area) const
  {
    return std::lower_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
  }

  typename memory_map_type::iterator
  find_by_address (const vm_area_interface* area)
  {
    return std::lower_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
  }

  // typename memory_map_type::iterator
  // find_by_size (size_t size)
  // {
  //   typename memory_map_type::iterator pos;
  //   for (pos = memory_map_.begin ();
  //   	 pos != memory_map_.end ();
  //   	 ++pos) {
  //     if ((*pos)->type () == VM_AREA_FREE && size <= (*pos)->size ()) {
  //   	break;
  //     }
  //   }
    
  //   return pos;
  // }

  // typename memory_map_type::iterator
  // merge (typename memory_map_type::iterator pos)
  // {
  //   if (pos != memory_map_.end ()) {
  //     typename memory_map_type::iterator next (pos);
  //     ++next;
  //     if (next != memory_map_.end ()) {
  //   	vm_area_interface* temp = *next;
  //   	if ((*pos)->merge (*temp)) {
  //   	  memory_map_.erase (next);
  //   	  destroy (temp, alloc_);
  //   	}
  //     }
  //   }
    
  //   return pos;
  // }

  // void
  // insert_into_free_area (typename memory_map_type::iterator pos,
  // 			 vm_area_interface* area)
  // {
  //   kassert ((*pos)->type () == VM_AREA_FREE);
  //   kassert (area->size () < (*pos)->size ());
    
  //   const void* left_begin = (*pos)->begin ();
  //   const void* left_end = area->begin ();
    
  //   const void* right_begin = area->end ();
  //   const void* right_end = (*pos)->end ();
    
  //   // Take out the old entry.
  //   destroy (*pos, alloc_);
  //   pos = memory_map_.erase (pos);
    
  //   // Split the area.
  //   if (right_begin < right_end) {
  //     pos = merge (memory_map_.insert (pos, new (alloc_) vm_free_area (right_begin, right_end)));
  //   }
  //   pos = merge (memory_map_.insert (pos, area));
  //   if (left_begin < left_end) {
  //     pos = merge (memory_map_.insert (pos, new (alloc_) vm_free_area (left_begin, left_end)));
  //   }
    
  //   if (pos != memory_map_.begin ()) {
  //     merge (--pos);
  //   }
  // }

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
	     descriptor_constants::privilege_t privilege) :
    aid_ (aid)
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
    kassert (0);
    //return stack_pointer_;
  }
  
  physical_address_t
  page_directory () const
  {
    kassert (0);
    //return page_directory_;
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
    typename memory_map_type::iterator pos = find_by_address (area);

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
  
  // void
  // remove_vm_area (vm_area_interface* area)
  // {
  //   kassert (area != 0);
    
  //   typename memory_map_type::iterator pos = std::find (memory_map_.begin (), memory_map_.end (), area);
  //   kassert (pos != memory_map_.end ());
  //   memory_map_.erase (pos);
  //   destroy (area, system_alloc ());
  // }

  void*
  alloc (size_t size) __attribute__((warn_unused_result))
  {
    kassert (0);
    // kassert (size > 0);
    
    // typename memory_map_type::iterator pos = find_by_size (size);
    // if (pos != memory_map_.end ()) {
    //   void* retval = const_cast<void*> ((*pos)->begin ());
    //   insert_into_free_area (pos, new (alloc_) vm_data_area (retval, static_cast<uint8_t*> (retval) + size, page_privilege_));
    //   return retval;
    // }
    // else {
    //   return 0;
    // }
  }

  void*
  reserve (size_t size)
  {
    kassert (0);
    // kassert (size > 0);
    
    // typename memory_map_type::iterator pos = find_by_size (size);
    
    // if (pos != memory_map_.end ()) {
    //   void* retval = const_cast<void*> ((*pos)->begin ());
    //   insert_into_free_area (pos, new (alloc_) vm_reserved_area (retval, static_cast<uint8_t*> (retval) + size));
    //   return retval;
    // }
    // else {
    //   return 0;
    // }
  }
  
  void
  unreserve (const void* address)
  {
    kassert (0);
    // kassert (address != 0);
    // kassert (is_aligned (address, PAGE_SIZE));
    
    // vm_reserved_area k (address, static_cast<const uint8_t*> (address) + PAGE_SIZE);
    
    // typename memory_map_type::iterator pos = find_by_address (&k);
    // kassert (pos != memory_map_.end ());
    // kassert ((*pos)->begin () == address);
    // kassert ((*pos)->type () == VM_AREA_RESERVED);
    
    // vm_free_area* f = new (alloc_) vm_free_area ((*pos)->begin (), (*pos)->end ());
    
    // destroy (*pos, alloc_);
    // pos = memory_map_.insert (memory_map_.erase (pos), f);
    
    // merge (pos);
    // if (pos != memory_map_.begin ()) {
    //   merge (--pos);
    // }
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
