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

#include "vm_area.hpp"
#include "descriptor.hpp"
#include "gdt.hpp"
#include "action.hpp"
#include "buffer.hpp"
#include "unordered_map.hpp"
#include "unordered_set.hpp"
#include "mapped_area.hpp"
#include "lily/syscall.h"

// The stack.
static const logical_address_t STACK_END = KERNEL_VIRTUAL_BASE;
static const logical_address_t STACK_BEGIN = STACK_END - PAGE_SIZE;

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

  struct input_act_hash {
    size_t
    operator() (const input_act& act) const
    {
      return caction_hash () (act.input);
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

  struct binding_hash {
    size_t
    operator() (const binding& b) const
    {
      return caction_hash () (b.output) ^ caction_hash () (b.input);
    }
  };

  aid_t aid_;
  bool privileged_;
  // Physical address that contains the automaton's page directory.
  physical_address_t const page_directory_;
  // Map from action entry point (aep) to action.
  typedef unordered_map<const void*, const paction* const> aep_to_action_map_type;
  aep_to_action_map_type aep_to_action_map_;
  // Map from name to action.
  typedef unordered_map<kstring, const paction* const, kstring_hash> name_to_action_map_type;
  name_to_action_map_type name_to_action_map_;
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef vector<vm_area_base*> memory_map_type;
  memory_map_type memory_map_;
  // Heap area.
  vm_area_base* heap_area_;
  // Stack area.
  vm_area_base* stack_area_;
  // Memory mapped areas.
  vector<mapped_area*> mapped_areas_;
  // Bound outputs.
public:
  typedef unordered_set <input_act, input_act_hash> input_action_set_type;
private:
  typedef unordered_map <caction, input_action_set_type, caction_hash> bound_outputs_map_type;
  bound_outputs_map_type bound_outputs_map_;

  // Bound inputs.
  typedef unordered_map<caction, output_act, caction_hash> bound_inputs_map_type;
  bound_inputs_map_type bound_inputs_map_;

  // Bindings.
  typedef unordered_set<binding, binding_hash> bindings_set_type;
  bindings_set_type bindings_set_;

  // Next bid to allocate.
  bid_t current_bid_;
  // Map from bid_t to buffer*.
  typedef unordered_map<bid_t, buffer*> bid_to_buffer_map_type;
  bid_to_buffer_map_type bid_to_buffer_map_;

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
    memory_map_type::const_iterator pos = upper_bound (memory_map_.begin (), memory_map_.end (), &k, compare_vm_area ());
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
    memory_map_type::iterator pos = upper_bound (memory_map_.begin (), memory_map_.end (), &k, compare_vm_area ());
    // We know that area->begin () < (*pos)->begin ().

    if (pos != memory_map_.begin ()) {
      --pos;
      if (address < (*pos)->end ()) {
	return pos;
      }
    }
    
    return memory_map_.end ();
  }

public:

  automaton (aid_t aid,
	     bool privileged,
	     frame_t page_directory_frame) :
    aid_ (aid),
    privileged_ (privileged),
    page_directory_ (frame_to_physical_address (page_directory_frame)),
    heap_area_ (0),
    stack_area_ (0),
    current_bid_ (0)
  { }

  ~automaton ()
  {
    // BUG:  Destroy vm_areas, buffers, etc.
    kassert (0);
  }

  aid_t
  aid () const
  {
    return aid_;
  }

  bool
  privileged () const
  {
    return privileged_;
  }

  physical_address_t
  page_directory_physical_address () const
  {
    return page_directory_;
  }

  logical_address_t
  stack_pointer () const
  {
    return stack_area_->end ();
  }

  bool
  vm_area_is_free (logical_address_t begin,
		   logical_address_t end)
  {
    vm_area_base area (begin, end);
    
    // Find the location to insert.
    memory_map_type::iterator pos = upper_bound (memory_map_.begin (), memory_map_.end (), &area, compare_vm_area ());
    // We know that area->begin () < (*pos)->begin ().

    // Ensure that the areas don't conflict.
    if (pos != memory_map_.begin ()) {
      memory_map_type::const_iterator prev = pos - 1;
      if (area.begin () < (*prev)->end ()) {
	return false;
      }
    }

    if (pos != memory_map_.end ()) {
      if ((*pos)->begin () < area.end ()) {
	return false;
      }
    }
    
    return true;
  }

  void
  insert_vm_area (vm_area_base* area)
  {
    kassert (area != 0);
    
    // Find the location to insert.
    memory_map_type::iterator pos = upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    memory_map_.insert (pos, area);
  }

  bool
  insert_heap_and_stack ()
  {
    kassert (heap_area_ == 0);
    kassert (stack_area_ == 0);

    if (memory_map_.empty ()) {
      // No virtual memory areas.
      return false;
    }

    if (!vm_area_is_free (STACK_BEGIN, STACK_END)) {
      // No space for a stack.
      return false;
    }

    heap_area_ = new vm_area_base (memory_map_.back ()->end (), memory_map_.back ()->end ());
    insert_vm_area (heap_area_);

    stack_area_ = new vm_area_base (STACK_BEGIN, STACK_END);
    insert_vm_area (stack_area_);

    return true;
  }
  
  void
  map_heap_and_stack ()
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);
    kassert (page_directory_ == vm::get_directory ());

    if (!is_aligned (heap_area_->begin (), PAGE_SIZE)) {
      // The heap is not page aligned.
      // We are going to remap it copy-on-write.
      vm::remap (heap_area_->begin (), vm::USER, vm::MAP_COPY_ON_WRITE);
    }

    // Map the stack using copy-on-write of the zero page.
    for (logical_address_t address = stack_area_->begin (); address != stack_area_->end (); address += PAGE_SIZE) {
      vm::map (address, vm::zero_frame (), vm::USER, vm::MAP_COPY_ON_WRITE, false);
    }
  }

  void
  insert_mapped_area (mapped_area* area)
  {
    insert_vm_area (area);
    mapped_areas_.push_back (area);
  }

  void*
  sbrk (ptrdiff_t size)
  {
    kassert (heap_area_ != 0);

    if (size > 0) {
      logical_address_t const old_end = heap_area_->end ();
      logical_address_t const new_end = old_end + size;
      // Find the heap.
      memory_map_type::const_iterator pos = find (memory_map_.begin (), memory_map_.end (), heap_area_);
      kassert (pos != memory_map_.end ());
      // Move to the next.
      ++pos;
      kassert (pos != memory_map_.end ());
      if (new_end <= (*pos)->begin ()) {
	// The allocation does not interfere with next area.  Success.
	heap_area_->set_end (new_end);

	// Map the zero frame.
	for (logical_address_t address = align_up (old_end, PAGE_SIZE); address < new_end; address += PAGE_SIZE) {
	  vm::map (address, vm::zero_frame (), vm::USER, vm::MAP_COPY_ON_WRITE, false);
	}

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
      // BUG:  Negative sbrk argument.
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
    kassert (size != 0);
    const logical_address_t address = reinterpret_cast<logical_address_t> (ptr);
    memory_map_type::const_iterator pos = find_address (address);
    return pos != memory_map_.end () && (*pos)->begin () <= address && (address + size) <= (*pos)->end ();
  }
  
  // void
  // page_fault (logical_address_t address,
  // 	      vm::page_fault_error_t error,
  // 	      volatile registers* regs)
  // {
  //   kassert (stub_area_ != 0);
  //   memory_map_type::const_iterator pos = find_address (address);
  //   if (pos != memory_map_.end ()) {
  //     (*pos)->page_fault (address, error, regs, stub_area_->begin ());
  //   }
  //   else {
  //     kout << "Page Fault" << endl;
  //     kout << "address = " << hexformat (address) << " error = " << hexformat (error) << endl;
  //     kout << *regs << endl;
  //     // BUG:  Handle page fault for address not in memory map.
  //     kassert (0);
  //   }
  // }

  bool
  add_action (paction* action)
  {
    kassert (action != 0);
    kassert (action->automaton == this);

    if (name_to_action_map_.find (action->name) == name_to_action_map_.end () &&
	aep_to_action_map_.find (action->action_entry_point) == aep_to_action_map_.end ()) {
      name_to_action_map_.insert (make_pair (action->name, action));
      aep_to_action_map_.insert (make_pair (action->action_entry_point, action));
      return true;
    }
    else {
      return false;
    }
  }

  const paction*
  find_action (const void* action_entry_point) const
  {
    aep_to_action_map_type::const_iterator pos = aep_to_action_map_.find (action_entry_point);
    if (pos != aep_to_action_map_.end ()) {
      return pos->second;
    }
    else {
      return 0;
    }
  }

  const paction*
  find_action (const kstring& name) const
  {
    name_to_action_map_type::const_iterator pos = name_to_action_map_.find (name);
    if (pos != name_to_action_map_.end ()) {
      return pos->second;
    }
    else {
      return 0;
    }
  }

  bool
  is_output_bound_to_automaton (const caction& output_action,
				const automaton* input_automaton) const
  {
    kassert (output_action.action->automaton == this);

    bound_outputs_map_type::const_iterator pos1 = bound_outputs_map_.find (output_action);
    if (pos1 != bound_outputs_map_.end ()) {
      // TODO:  Replace this iteration with look-up in a data structure.
      for (input_action_set_type::const_iterator pos2 = pos1->second.begin (); pos2 != pos1->second.end (); ++pos2) {
	if (pos2->input.action->automaton == input_automaton) {
	  return true;
	}
      }
    }

    return false;
  }

  void
  bind_output (const caction& output_action,
	       const caction& input_action,
	       automaton* owner)
  {
    kassert (output_action.action->automaton == this);

    pair<bound_outputs_map_type::iterator, bool> r = bound_outputs_map_.insert (make_pair (output_action, input_action_set_type ()));
    r.first->second.insert (input_act (input_action, owner));
  }
  
  bool
  is_input_bound (const caction& input_action) const
  {
    kassert (input_action.action->automaton == this);

    return bound_inputs_map_.find (input_action) != bound_inputs_map_.end ();
  }

  void
  bind_input (const caction& output_action,
	      const caction& input_action,
	      automaton* owner)
  {
    kassert (input_action.action->automaton == this);

    bound_inputs_map_.insert (make_pair (input_action, output_act (output_action, owner)));
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
		 const void* parameter) const
  {
    aep_to_action_map_type::const_iterator pos = aep_to_action_map_.find (aep);
    if (pos != aep_to_action_map_.end ()) {
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
      bid = max (bid + 1, 0); // Handles overflow.
    }
    current_bid_ = max (bid + 1, 0);
    return bid;
  }

public:
  // BUG:  Limit the number of buffers.

  bid_t
  buffer_create (size_t size)
  {
    size = align_up (size, PAGE_SIZE);

    // Generate an id.
    bid_t bid = generate_bid ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new buffer (size);
    bid_to_buffer_map_.insert (make_pair (bid, b));
    
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
	buffer* n = new buffer (*b, offset, length);
	bid_to_buffer_map_.insert (make_pair (bid, n));
	
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
    buffer* b = new buffer (other);
    bid_to_buffer_map_.insert (make_pair (bid, b));
    
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

  pair<void*, int>
  buffer_map (bid_t bid)
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);

    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos == bid_to_buffer_map_.end ()) {
      // The buffer does not exist.
      return make_pair ((void*)0, LILY_SYSCALL_EBADBID);
    }

    buffer* b = bpos->second;
    
    if (b->size () == 0) {
      // The buffer is empty.
      return make_pair ((void*)0, LILY_SYSCALL_EBADBID);
    }

    if (b->begin () != 0) {
      // The buffer is already mapped.  Return the address.
      return make_pair (reinterpret_cast<void*> (b->begin ()), LILY_SYSCALL_ESUCCESS);
    }

    // Map the buffer.
    
    // Find the heap.
    memory_map_type::const_reverse_iterator heap_pos = memory_map_type::const_reverse_iterator (find (memory_map_.begin (), memory_map_.end (), heap_area_));
    
    kassert (heap_pos != memory_map_.rbegin ());
    --heap_pos;
    kassert (heap_pos != memory_map_.rend ());
    
    // Find the stack.
    memory_map_type::reverse_iterator stack_pos = find (memory_map_.rbegin (), memory_map_.rend (), stack_area_);
    kassert (stack_pos != memory_map_.rend ());
    
    // Find a hole and map.
    for (; stack_pos != heap_pos; ++stack_pos) {
      memory_map_type::reverse_iterator prev = stack_pos + 1;
      size_t size = (*stack_pos)->begin () - (*prev)->end ();
      if (size >= b->size ()) {
	b->map ((*stack_pos)->begin ());
	memory_map_.insert (prev.base (), b);
	// Success.
	return make_pair (reinterpret_cast<void*> (b->begin ()), LILY_SYSCALL_ESUCCESS);
      }
    }
    
    // Couldn't find a big enough hole.
    return make_pair ((void*)0, LILY_SYSCALL_ENOMEM);
  }

  int
  buffer_unmap (bid_t bid)
  {
    bid_to_buffer_map_type::iterator bpos = bid_to_buffer_map_.find (bid);
    if (bpos != bid_to_buffer_map_.end ()) {
      buffer* b = bpos->second;

      if (b->begin () != 0) {
	// Remove from the memory map.
	memory_map_.erase (find_address (b->begin ()));

	// Unmap the buffer.	
	b->unmap ();

	return 0;
      }
      else {
	// The buffer was not mapped.
	return -1;
      }
    }
    else {
      // The buffer does not exist.
      return -1;
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
      delete b;

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

  buffer*
  lookup_buffer (bid_t bid)
  {
    bid_to_buffer_map_type::const_iterator bpos = bid_to_buffer_map_.find (bid);
    kassert (bpos != bid_to_buffer_map_.end ());
    return bpos->second;
  }

};

#endif /* __automaton_hpp__ */
