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
  // Automaton identifier.
  aid_t aid_;
  // Frame that contains the automaton's page directory.
  frame_t const page_directory_frame_;
  // Table of action descriptors for guiding execution, checking bindings, etc.
  typedef std::unordered_map<const void*, paction, std::hash<const void*>, std::equal_to<const void*>, system_allocator<std::pair<const void* const, paction> > > action_map_type;
  action_map_type action_map_;
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef std::vector<vm_area_base*, system_allocator<vm_area_base*> > memory_map_type;
  memory_map_type memory_map_;
  // Heap area.
  vm_heap_area* heap_area_;
  // Stack area.
  vm_stack_area* stack_area_;
  // Next bid to allocate.
  bid_t current_bid_;
  // Map from bid_t to buffer*.
  typedef std::unordered_map<bid_t, buffer*, std::hash<bid_t>, std::equal_to<bid_t>, system_allocator<std::pair<const bid_t, buffer*> > > bid_to_buffer_map_type;
  bid_to_buffer_map_type bid_to_buffer_map_;

public:
  class const_action_iterator : public std::iterator<std::bidirectional_iterator_tag, paction> {
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

    const paction*
    operator-> () const
    {
      return &(pos_->second);
    }

    const paction&
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
  find_address (const void* address) const
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
  find_address (const void* address)
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
    paction ac (Action::action_type, action_entry_point, Action::parameter_mode, Action::buffer_value_mode, Action::copy_value_mode, Action::copy_value_size);
    std::pair<typename action_map_type::iterator, bool> r = action_map_.insert (std::make_pair (ac.action_entry_point, ac));
    kassert (r.second);
  }

public:

  automaton (aid_t aid,
	     frame_t page_directory_frame) :
    aid_ (aid),
    page_directory_frame_ (page_directory_frame),
    heap_area_ (0),
    stack_area_ (0),
    current_bid_ (0)
  { }

  ~automaton ()
  {
    // Destroy buffers that have been created but not mapped.
    for (bid_to_buffer_map_type::iterator pos = bid_to_buffer_map_.begin ();
	 pos != bid_to_buffer_map_.end ();
	 ++pos) {
      destroy (pos->second, system_alloc ());
    }
  }

  bool
  address_in_use (const void* address) const
  {
    return find_address (address) != memory_map_.end ();
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
  
  frame_t
  page_directory_frame () const
  {
    return page_directory_frame_;
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
      heap_area_->set_end (new_end);
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
	      page_fault_error_t error,
	      volatile registers* regs)
  {
    memory_map_type::const_iterator pos = find_address (address);
    if (pos != memory_map_.end ()) {
      (*pos)->page_fault (address, error, regs);
    }
    else {
      kout << "Page Fault" << endl;
      kout << "aid = " << aid_ << " address = " << hexformat (address) << " error = " << hexformat (error) << endl;
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
  buffer_create (bid_t other,
		 size_t offset,
		 size_t length)
  {
    offset = align_down (offset, PAGE_SIZE);
    length = align_up (length, PAGE_SIZE);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (other);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;
      if (offset + length <= b->size ()) {
	if (length == 0) {
	  // Correct the length.
	  length = b->size () - offset;
	}

	// Make the source copy-on-write.
	b->make_copy_on_write ();
	
	// Generate an id.
	bid_t bid = generate_bid ();
	
	// Create the buffer and insert it into the map.
	buffer* b = new (system_alloc ()) buffer (*b, offset, length);
	bid_to_buffer_map_.insert (std::make_pair (bid, b));
	
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
  buffer_append (bid_t bid,
		 size_t size)
  {
    size = align_up (size, PAGE_SIZE);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;
      if (b->begin () == 0) {
	return b->append (size);
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
	if (length == 0) {
	  // Correct the length.
	  length = src->size () - offset;
	}

	if (dst->begin () == 0) {
	  // Make the source copy on write so we won't see subsequent changes.
	  src->make_copy_on_write ();
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

  void*
  buffer_map (bid_t bid)
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;

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
	  size_t size = static_cast<const uint8_t*> ((*stack_pos)->begin ()) - static_cast<const uint8_t*> ((*prev)->end ());
	  if (size >= b->size ()) {
	    b->set_end ((*stack_pos)->begin ());
	    memory_map_.insert (prev.base (), b);
	    return const_cast<void*> (b->begin ());
	  }
	}
	
	// Couldn't find a big enough hole.
	return reinterpret_cast<void*> (-1);
      }
      else {
	// The buffer is already mapped.  Return the address.
	return const_cast<void*> (b->begin ());
      }
    }
    else {
      // The buffer does not exist.
      return reinterpret_cast<void*> (-1);
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
