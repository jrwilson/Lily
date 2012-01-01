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
#include <unordered_set>
#include <vector>
#include "descriptor.hpp"
#include "action_traits.hpp"
#include "static_assert.hpp"
#include <type_traits>
#include "gdt.hpp"
#include "action.hpp"
#include "bid.hpp"
#include "buffer.hpp"

class automaton {
private:
  struct input_act {
    caction const input;
    automaton* const owner;

    input_act (const caction& i,
	       automaton* own) :
      input (i),
      owner (own)
    { }

    inline bool
    operator== (const input_act& other) const
    {
      // Owner doesn't matter.
      return input == other.input;
    }
  };

  struct output_act {
    caction const output;
    automaton* const owner;

    output_act (const caction& o,
		automaton* own) :
      output (o),
      owner (own)
    { }
  };

  struct binding {
    caction const output;
    caction const input;

    binding (const caction& o,
	     const caction& i) :
      output (o),
      input (i)
    { }

    inline bool
    operator== (const binding& other) const
    {
      return output == other.output && input == other.input;
    }
  };

  aid_t aid_;
  // Frame that contains the automaton's page directory.
  frame_t const page_directory_frame_;
  // Flag indicating if the automaton can execute privileged instructions.
  bool privileged_;
  // Table of action descriptors for guiding execution, checking bindings, etc.
  typedef std::unordered_map<const void*, const paction* const, std::hash<const void*>, std::equal_to<const void*>, system_allocator<std::pair<const void* const, const paction* const> > > action_map_type;
  action_map_type action_map_;
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef std::vector<vm_area_base*, system_allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  // Heap area.
  vm_heap_area* heap_area_;
  // Stack area.
  vm_stack_area* stack_area_;

  // Bound outputs.
public:
  typedef std::unordered_set <input_act, std::hash<input_act>, std::equal_to<input_act>, system_allocator<input_act> > input_action_set_type;
private:
  typedef std::unordered_map <caction, input_action_set_type, std::hash<caction>, std::equal_to<caction>, system_allocator<std::pair<const caction, input_action_set_type> > > bound_outputs_map_type;
  bound_outputs_map_type bound_outputs_map_;

  // Bound inputs.
  typedef std::unordered_map<caction, output_act, std::hash<caction>, std::equal_to<caction>, system_allocator<std::pair<const caction, output_act> > > bound_inputs_map_type;
  bound_inputs_map_type bound_inputs_map_;

  // Bindings.
  typedef std::unordered_set<binding, std::hash<binding>, std::equal_to<binding>, system_allocator<binding> > bindings_set_type;
  bindings_set_type bindings_set_;

  // Next bid to allocate.
  bid_t current_bid_;
  // Map from bid_t to buffer*.
  typedef std::unordered_map<bid_t, buffer*, std::hash<bid_t>, std::equal_to<bid_t>, system_allocator<std::pair<const bid_t, buffer*> > > bid_to_buffer_map_type;
  bid_to_buffer_map_type bid_to_buffer_map_;

public:
  class const_action_iterator : public std::iterator<std::bidirectional_iterator_tag, const paction* const> {
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

    const paction* const *
    operator-> () const
    {
      return &(pos_->second);
    }

    const paction*const &
    operator* () const
    {
      return (pos_->second);
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

  memory_map_type::const_iterator
  find_address (logical_address_t address) const
  {
    vm_area_base k (address, address);
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
  find_address (logical_address_t address)
  {
    vm_area_base k (address, address);
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
    paction* ac = new (system_alloc ()) paction (this, Action::action_type, action_entry_point, Action::parameter_mode, Action::buffer_value_mode, Action::copy_value_mode, Action::copy_value_size);
    std::pair<typename action_map_type::iterator, bool> r = action_map_.insert (std::make_pair (ac->action_entry_point, ac));
    kassert (r.second);
  }

public:

  automaton (aid_t aid,
	     frame_t page_directory_frame,
	     bool privileged) :
    aid_ (aid),
    page_directory_frame_ (page_directory_frame),
    privileged_ (privileged),
    heap_area_ (0),
    stack_area_ (0),
    current_bid_ (0)
  { }

  ~automaton ()
  {
    // TODO
    kassert (0);
    // Destroy buffers that have been created but not mapped.
    for (bid_to_buffer_map_type::iterator pos = bid_to_buffer_map_.begin ();
	 pos != bid_to_buffer_map_.end ();
	 ++pos) {
      destroy (pos->second, system_alloc ());
    }
  }

  aid_t
  aid () const
  {
    return aid_;
  }

  frame_t
  page_directory_frame () const
  {
    return page_directory_frame_;
  }

  bool
  can_execute_privileged () const
  {
    return privileged_;
  }

  logical_address_t
  stack_pointer () const
  {
    kassert (stack_area_ != 0);
    return stack_area_->end ();
  }

  // TODO:  Return an iterator.
  bool
  insert_vm_area (vm_area_base* area)
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

    if (size > 0) {
      logical_address_t const old_end = heap_area_->end ();
      logical_address_t const new_end = old_end + size;
      // Find the heap.
      memory_map_type::const_iterator pos = std::find (memory_map_.begin (), memory_map_.end (), heap_area_);
      kassert (pos != memory_map_.end ());
      // Move to the next.
      ++pos;
      if (new_end <= (*pos)->begin ()) {
	// The allocation does not interfere with next area.  Success.
	heap_area_->set_end (new_end);
	return reinterpret_cast<void*> (old_end);
      }
      else {
	// Failure.
	return 0;
      }
    }
    else if (size < 0) {
      // if (new_end < heap_area_->begin ()) {
      // 	// Shrunk too much.
      // 	// Fail.
      // 	return 0;
      // }
      // TODO:  Negative sbrk argument.
      kassert (0);
      return 0;
    }
    else {
      return reinterpret_cast<void*> (heap_area_->end ());
    }
  }

  bool
  verify_span (const void* ptr,
	       size_t size) const
  {
    const logical_address_t address = reinterpret_cast<logical_address_t> (ptr);
    memory_map_type::const_iterator pos = find_address (address);
    return pos != memory_map_.end () && (*pos)->begin () <= address && (address + size) <= (*pos)->end ();
  }
  
  void
  page_fault (logical_address_t address,
	      page_fault_error_t error,
	      volatile registers* regs)
  {
    memory_map_type::const_iterator pos = find_address (address);
    if (pos != memory_map_.end ()) {
      (*pos)->page_fault (address, error, regs);
    }
    else {
      kout << "Page Fault" << endl;
      kout << "address = " << hexformat (address) << " error = " << hexformat (error) << endl;
      kout << *regs << endl;
      // TODO:  Handle page fault for address not in memory map.
      kassert (0);
    }
  }

private:
  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			no_parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (void))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			no_parameter_tag,
			no_buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (typename Action::copy_value_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			no_parameter_tag,
			buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (bid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			no_parameter_tag,
			buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (bid_t, typename Action::copy_value_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			parameter_tag,
			no_buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type, typename Action::copy_value_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			parameter_tag,
			buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type, bid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			parameter_tag,
			buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type, bid_t, typename Action::copy_value_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			auto_parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (aid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			auto_parameter_tag,
			no_buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (aid_t, typename Action::copy_value_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			auto_parameter_tag,
			buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (aid_t, bid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (input_action_tag,
			auto_parameter_tag,
			buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (aid_t, bid_t, typename Action::copy_value_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			no_parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (void))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			no_parameter_tag,
			no_buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (void))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			no_parameter_tag,
			buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (void))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			no_parameter_tag,
			buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (void))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			parameter_tag,
			no_buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			parameter_tag,
			buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			parameter_tag,
			buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			auto_parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (aid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			auto_parameter_tag,
			no_buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (aid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			auto_parameter_tag,
			buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (aid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (output_action_tag,
			auto_parameter_tag,
			buffer_value_tag,
			copy_value_tag,
			void (*action_entry_point) (aid_t))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (internal_action_tag,
			no_parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (void))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

  template <class Action>
  void
  add_action_dispatch_ (internal_action_tag,
			parameter_tag,
			no_buffer_value_tag,
			no_copy_value_tag,
			void (*action_entry_point) (typename Action::parameter_type))
  {
    add_action_<Action> (reinterpret_cast<const void*> (action_entry_point));
  }

public:
  // TODO:  Remove template code.
  template <class Action>
  void
  add_action (void (*action_entry_point) ())
  {
    STATIC_ASSERT (is_action<Action>::value);
    add_action_dispatch_<Action> (typename Action::action_category (),
				  typename Action::parameter_category (),
				  typename Action::buffer_value_category (),
				  typename Action::copy_value_category (),
				  action_entry_point);
  }
  
  template <class Action,
	    class T>
  void
  add_action (void (*action_entry_point) (T))
  {
    STATIC_ASSERT (is_action<Action>::value);
    add_action_dispatch_<Action> (typename Action::action_category (),
				  typename Action::parameter_category (),
				  typename Action::buffer_value_category (),
				  typename Action::copy_value_category (),
				  action_entry_point);
  }

  template <class Action,
	    class T1,
	    class T2>
  void
  add_action (void (*action_entry_point) (T1, T2))
  {
    STATIC_ASSERT (is_action<Action>::value);
    add_action_dispatch_<Action> (typename Action::action_category (),
				  typename Action::parameter_category (),
				  typename Action::buffer_value_category (),
				  typename Action::copy_value_category (),
				  action_entry_point);
  }

  template <class Action,
	    class T1,
	    class T2,
	    class T3>
  void
  add_action (void (*action_entry_point) (T1, T2, T3))
  {
    STATIC_ASSERT (is_action<Action>::value);
    add_action_dispatch_<Action> (typename Action::action_category (),
				  typename Action::parameter_category (),
				  typename Action::buffer_value_category (),
				  typename Action::copy_value_category (),
				  action_entry_point);
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

  void
  bind_output (const caction& output_action,
	       const caction& input_action,
	       automaton* owner)
  {
    std::pair<bound_outputs_map_type::iterator, bool> r = bound_outputs_map_.insert (std::make_pair (output_action, input_action_set_type ()));
    r.first->second.insert (input_act (input_action, owner));
  }
  
  void
  bind_input (const caction& output_action,
	      const caction& input_action,
	      automaton* owner)
  {
    bound_inputs_map_.insert (std::make_pair (input_action, output_act (output_action, owner)));
  }

  void
  bind (const caction& output_action,
	const caction& input_action)
  {
    bindings_set_.insert (binding (output_action, input_action));
  }

  const input_action_set_type*
  get_bound_inputs (const caction& output_action) const
  {
    bound_outputs_map_type::const_iterator pos = bound_outputs_map_.find (output_action);
    if (pos != bound_outputs_map_.end ()) {
      return &pos->second;
    }
    else {
      return 0;
    }
  }

  size_t
  binding_count (const void* aep,
		 aid_t parameter) const
  {
    action_map_type::const_iterator pos = action_map_.find (aep);
    if (pos != action_map_.end ()) {
      const caction act (pos->second, parameter);
      switch (pos->second->type) {
      case OUTPUT:
	{
	  bound_outputs_map_type::const_iterator pos2 = bound_outputs_map_.find (act);
	  if (pos2 != bound_outputs_map_.end ()) {
	    return pos2->second.size ();
	  }
	  else {
	    return 0;
	  }
	}
	break;
      case INPUT:
	if (bound_inputs_map_.find (act) != bound_inputs_map_.end ()) {
	  return 1;
	}
	else {
	  return 0;
	}
	break;
      case INTERNAL:
	return 0;
	break;
      }
    }
    // Action does not exist.
    return -1;
  }

private:
  bid_t
  generate_bid ()
  {
    // Generate an id.
    bid_t bid = current_bid_;
    while (bid_to_buffer_map_.find (bid) != bid_to_buffer_map_.end ()) {
      bid = std::max (bid + 1, 0); // Handles overflow.
    }
    current_bid_ = std::max (bid + 1, 0);
    return bid;
  }

public:
  // TODO:  Limit the number of buffers.

  bid_t
  buffer_create (size_t size)
  {
    size = align_up (size, PAGE_SIZE);

    // Generate an id.
    bid_t bid = generate_bid ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new (system_alloc ()) buffer (size);
    bid_to_buffer_map_.insert (std::make_pair (bid, b));
    
    return bid;
  }
  
  bid_t
  buffer_copy (bid_t other,
	       size_t offset,
	       size_t length)
  {
    offset = align_down (offset, PAGE_SIZE);
    length = align_up (length, PAGE_SIZE);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (other);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;
      if (offset + length <= b->size ()) {
	// Generate an id.
	bid_t bid = generate_bid ();
	
	// Create the buffer and insert it into the map.
	buffer* n = new (system_alloc ()) buffer (*b, offset, length);
	bid_to_buffer_map_.insert (std::make_pair (bid, n));
	
	return bid;
      }
      else {
	// Offset is past end of buffer.
	return -1;
      }
    }
    else {
      // Buffer does not exist.
      return -1;
    }
  }

  bid_t
  buffer_create (const buffer& other)
  {
    // Generate an id.
    bid_t bid = generate_bid ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new (system_alloc ()) buffer (other);
    bid_to_buffer_map_.insert (std::make_pair (bid, b));
    
    return bid;
  }

  size_t
  buffer_grow (bid_t bid,
	       size_t size)
  {
    size = align_up (size, PAGE_SIZE);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;
      if (b->begin () == 0) {
	return b->grow (size);
      }
      else {
	// Buffer was mapped.
	return -1;
      }
    }
    else {
      // Buffer does not exist.
      return -1;
    }
  }

  size_t
  buffer_append (bid_t dst,
		 bid_t src,
		 size_t offset,
		 size_t length)
  {
    offset = align_down (offset, PAGE_SIZE);
    length = align_up (length, PAGE_SIZE);

    bid_to_buffer_map_type::const_iterator dst_pos = bid_to_buffer_map_.find (dst);
    bid_to_buffer_map_type::const_iterator src_pos = bid_to_buffer_map_.find (src);
    if (dst_pos != bid_to_buffer_map_.end () &&
	src_pos != bid_to_buffer_map_.end ()) {
      buffer* dst = dst_pos->second;
      buffer* src = src_pos->second;
      if (offset + length <= src->size ()) {
	if (dst->begin () == 0) {
	  // Append.
	  return dst->append (*src, offset, length);
	}
	else {
	  // The destination is mapped.
	  return -1;
	}
      }
      else {
	// Offset is past end of source.
	return -1;
      }
    }
    else {
      // One of the buffers does not exist.
      return -1;
    }
  }

  int
  buffer_assign (bid_t dest,
		 size_t dest_offset,
		 bid_t src,
		 size_t src_offset,
		 size_t length)
  {
    bid_to_buffer_map_type::const_iterator dest_pos = bid_to_buffer_map_.find (dest);
    bid_to_buffer_map_type::const_iterator src_pos = bid_to_buffer_map_.find (src);
    if (dest_pos != bid_to_buffer_map_.end () &&
	src_pos != bid_to_buffer_map_.end ()) {
      buffer* dest_b = dest_pos->second;
      buffer* src_b = src_pos->second;
      if (dest_offset + length <= dest_b->size () &&
	  src_offset + length <= src_b->size ()) {
	// Assign.
	dest_b->assign (dest_offset, *src_b, src_offset, length);
	return 0;
      }
      else {
	// Would got outside of range.
	return -1;
      }
    }
    else {
      // One of the buffers does not exist.
      return -1;
    }
  }

  void*
  buffer_map (bid_t bid)
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;

      if (b->size () != 0) {
	if (b->begin () == 0) {
	  // Map the buffer.
	  
	  // Find the heap.
	  memory_map_type::const_reverse_iterator heap_pos = memory_map_type::const_reverse_iterator (std::find (memory_map_.begin (), memory_map_.end (), heap_area_));
	  kassert (heap_pos != memory_map_.rbegin ());
	  --heap_pos;
	  kassert (heap_pos != memory_map_.rend ());
	  
	  // Find the stack.
	  memory_map_type::reverse_iterator stack_pos = std::find (memory_map_.rbegin (), memory_map_.rend (), stack_area_);
	  kassert (stack_pos != memory_map_.rend ());
	  
	  // Find a hole and map.
	  for (; stack_pos != heap_pos; ++stack_pos) {
	    memory_map_type::reverse_iterator prev = stack_pos + 1;
	    size_t size = (*stack_pos)->begin () - (*prev)->end ();
	    if (size >= b->size ()) {
	      b->set_end ((*stack_pos)->begin ());
	      memory_map_.insert (prev.base (), b);
	      return reinterpret_cast<void*> (b->begin ());
	    }
	  }
	  
	  // Couldn't find a big enough hole.
	  return 0;
	}
	else {
	  // The buffer is already mapped.  Return the address.
	  return reinterpret_cast<void*> (b->begin ());
	}
      }
      else {
	// The buffer has size == 0.
	return 0;
      }
    }
    else {
      // The buffer does not exist.
      return 0;
    }
  }

  buffer*
  buffer_output_destroy (bid_t bid)
  {
    bid_to_buffer_map_type::iterator bpos = bid_to_buffer_map_.find (bid);
    kassert (bpos != bid_to_buffer_map_.end ());

    buffer* b = bpos->second;
    
    if (b->begin () != 0) {
      // Remove from the memory map.
      memory_map_.erase (find_address (b->begin ()));
      
      // Unmap the buffer.	
      b->unmap ();
    }
    
    // Remove from the bid map.
    bid_to_buffer_map_.erase (bpos);
    
    return b;
  }

  int
  buffer_destroy (bid_t bid)
  {
    bid_to_buffer_map_type::iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;

      if (b->begin () != 0) {
	// Remove from the memory map.
	memory_map_.erase (find_address (b->begin ()));

	// Unmap the buffer.	
	b->unmap ();
      }

      // Remove from the bid map.
      bid_to_buffer_map_.erase (bpos);

      // Destroy it.
      destroy (b, system_alloc ());

      return 0;
    }
    else {
      // The buffer does not exist.
      return -1;
    }
  }

  size_t
  buffer_size (bid_t bid)
  {
    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      return bpos->second->size ();
    }
    else {
      // The buffer does not exist.
      return -1;
    }
  }

  bool
  buffer_exists (bid_t bid)
  {
    return bid_to_buffer_map_.find (bid) != bid_to_buffer_map_.end ();
  }

};

#endif /* __automaton_hpp__ */
