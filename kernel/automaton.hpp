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
#include "io.hpp"
#include "irq_handler.hpp"
#include "kstring.hpp"

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
  // Map from action number to action.
  typedef unordered_map<ano_t, const paction* const> ano_to_action_map_type;
  ano_to_action_map_type ano_to_action_map_;
  // Description of the actions.
  kstring description_;
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef vector<vm_area_base*> memory_map_type;
  memory_map_type memory_map_;
  // Heap area.
  vm_area_base* heap_area_;
  // Stack area.
  vm_area_base* stack_area_;
  // Memory mapped areas.
  vector<mapped_area*> mapped_areas_;
  // Reserved I/O ports.
  typedef unordered_set<unsigned short> port_set_type;
  port_set_type port_set_;
  // Interrupts.
  typedef unordered_map<int, caction> irq_map_type;
  irq_map_type irq_map_;
  // Automata to which this automaton is subscribed.
  typedef unordered_set<automaton*> subscriptions_set_type;
  subscriptions_set_type subscriptions_;
  // Automata that receive notification when this automaton is destroyed.
  typedef unordered_set<automaton*> subscribers_set_type;
  subscribers_set_type subscribers_;
  // Bound outputs.
public:
  typedef unordered_set <input_act, input_act_hash> input_action_set_type;
private:
  typedef unordered_map <caction, input_action_set_type, caction_hash> bound_outputs_map_type;
  bound_outputs_map_type bound_outputs_map_;

  // Bound inputs.
  typedef unordered_map<caction, output_act, caction_hash> bound_inputs_map_type;
  bound_inputs_map_type bound_inputs_map_;

  // Next binding id to allocate.
  bid_t current_bid_;
  typedef unordered_map<bid_t, binding> bid_to_binding_map_type;
  bid_to_binding_map_type bid_to_binding_map_;
  typedef unordered_set<binding, binding_hash> bindings_set_type;
  bindings_set_type bindings_set_;

  // Next buffer descriptor to allocate.
  bd_t current_bd_;
  // Map from bd_t to buffer*.
  typedef unordered_map<bd_t, buffer*> bd_to_buffer_map_type;
  bd_to_buffer_map_type bd_to_buffer_map_;

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
    current_bid_ (0),
    current_bd_ (0)
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

    // Map the stack.
    // We do not try to copy-on-write the zero page because copy-on-write only works in user mode and we write to the stack in supervisor mode.
    for (logical_address_t address = stack_area_->begin (); address != stack_area_->end (); address += PAGE_SIZE) {
      frame_t frame = frame_manager::alloc ();
      kassert (frame != vm::zero_frame ());
      vm::map (address, frame, vm::USER, vm::MAP_READ_WRITE, false);
    }
  }

  void
  insert_mapped_area (mapped_area* area)
  {
    insert_vm_area (area);
    mapped_areas_.push_back (area);
  }

  void
  reserve_port (unsigned short port)
  {
    port_set_.insert (port);
  }

  pair<unsigned char, int>
  inb (unsigned short port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inb (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  pair<int, int>
  outb (unsigned short port,
	unsigned char value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outb (port, value);
      return make_pair (0, LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  pair<int, int>
  subscribe_irq (int irq,
		 ano_t action_number,
		 int parameter)
  {
    if (!privileged ()) {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }

    if (irq < irq_handler::MIN_IRQ ||
	irq > irq_handler::MAX_IRQ) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }

    // Find the action.
    const paction* action = find_action (action_number);
    if (action == 0 || action->type != SYSTEM_INPUT) {
      return make_pair (-1, LILY_SYSCALL_EBADANO);
    }

    // Correct the parameter.
    if (action->parameter_mode == NO_PARAMETER) {
      parameter = 0;
    }

    if (irq_map_.find (irq) != irq_map_.end ()) {
      /* Already subscribed.  Not that we don't check the action. */
      return make_pair (0, LILY_SYSCALL_ESUCCESS);
    }

    caction c (action, parameter);

    irq_map_.insert (make_pair (irq, c));
    irq_handler::subscribe (irq, c);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  pair<int, int>
  syslog (const char* string,
	  size_t size)
  {
    if (!verify_span (string, size)) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }

    kout << aid_ << ": ";
    while (size != 0) {
      kout.put (*string);
      ++string;
      --size;
    }
    kout << endl;

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  pair<void*, int>
  adjust_break (ptrdiff_t size)
  {
    kassert (heap_area_ != 0);

    if (size != 0) {
      logical_address_t const old_end = heap_area_->end ();
      logical_address_t new_end = old_end + size;
      
      if (size > 0) {
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
	  
	  return make_pair (reinterpret_cast<void*> (old_end), LILY_SYSCALL_ESUCCESS);
	}
	else {
	  // Failure.
	  return make_pair ((void*)0, LILY_SYSCALL_ENOMEM);
	}
      }
      else {
	if (new_end < heap_area_->begin ()) {
	  // Can't shrink beyond the beginning of the heap.
	  new_end = heap_area_->begin ();
	}
	heap_area_->set_end (new_end);

	// Unmap.
	for (logical_address_t address = align_up (new_end, PAGE_SIZE); address < old_end; address += PAGE_SIZE) {
	  vm::unmap (address);
	}

	return make_pair (reinterpret_cast<void*> (old_end), LILY_SYSCALL_ESUCCESS);
      }
    }
    else {
      return make_pair (reinterpret_cast<void*> (heap_area_->end ()), LILY_SYSCALL_ESUCCESS);
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

    if (ano_to_action_map_.find (action->action_number) == ano_to_action_map_.end ()) {
      ano_to_action_map_.insert (make_pair (action->action_number, action));
      return true;
    }
    else {
      return false;
    }
  }

  void
  set_description (const kstring& desc)
  {
    description_ = desc;
  }

  const kstring&
  get_description (void) const
  {
    return description_;
  }

  const paction*
  find_action (ano_t ano) const
  {
    ano_to_action_map_type::const_iterator pos = ano_to_action_map_.find (ano);
    if (pos != ano_to_action_map_.end ()) {
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

  bid_t
  bind (const caction& output_action,
	const caction& input_action)
  {
    const binding b (output_action, input_action);

    // Generate an id.
    bid_t bid = generate_bid ();
    bid_to_binding_map_.insert (make_pair (bid, b));

    bindings_set_.insert (b);

    return bid;
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

  void
  add_subscriber (automaton* a)
  {
    subscribers_.insert (a);
  }

  void
  add_subscription (automaton* a)
  {
    subscriptions_.insert (a);
  }

private:
  bd_t
  generate_bd ()
  {
    // Generate an id.
    bd_t bd = current_bd_;
    while (bd_to_buffer_map_.find (bd) != bd_to_buffer_map_.end ()) {
      bd = max (bd + 1, 0); // Handles overflow.
    }
    current_bd_ = max (bd + 1, 0);
    return bd;
  }

  bid_t
  generate_bid ()
  {
    // Generate an id.
    bid_t bid = current_bid_;
    while (bid_to_binding_map_.find (bid) != bid_to_binding_map_.end ()) {
      bid = max (bid + 1, 0); // Handles overflow.
    }
    current_bid_ = max (bid + 1, 0);
    return bid;
  }

public:
  // BUG:  Limit the number of buffers.

  pair<bd_t, int>
  buffer_create (size_t size)
  {
    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new buffer (size);
    bd_to_buffer_map_.insert (make_pair (bd, b));

    return make_pair (bd, LILY_SYSCALL_ESUCCESS);
  }

  pair<bd_t, int>
  buffer_copy (bd_t other)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (other);
    if (bpos == bd_to_buffer_map_.end ()) {
      // Buffer does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }

    buffer* b = bpos->second;

    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* n = new buffer (*b, 0, b->size ());
    bd_to_buffer_map_.insert (make_pair (bd, n));
    
    return make_pair (bd, LILY_SYSCALL_ESUCCESS);
  }
  
  // pair<bd_t, int>
  // buffer_copy (bd_t other,
  // 	       size_t begin,
  // 	       size_t end)
  // {
  //   if (begin > end) {
  //     return make_pair (-1, LILY_SYSCALL_EINVAL);
  //   }

  //   bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (other);
  //   if (bpos == bd_to_buffer_map_.end ()) {
  //     // Buffer does not exist.
  //     return make_pair (-1, LILY_SYSCALL_EBDDNE);
  //   }

  //   buffer* b = bpos->second;
  //   if (end > b->size ()) {
  //     // End is past end of buffer.
  //     return make_pair (-1, LILY_SYSCALL_EINVAL);
  //   }

  //   // Generate an id.
  //   bd_t bd = generate_bd ();
    
  //   // Create the buffer and insert it into the map.
  //   buffer* n = new buffer (*b, begin, end);
  //   bd_to_buffer_map_.insert (make_pair (bd, n));
    
  //   return make_pair (bd, LILY_SYSCALL_ESUCCESS);
  // }

  bd_t
  buffer_create (const buffer& other)
  {
    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new buffer (other);
    bd_to_buffer_map_.insert (make_pair (bd, b));
    
    return bd;
  }
  
  pair<size_t, int>
  buffer_resize (bd_t bd,
		 size_t size)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos == bd_to_buffer_map_.end ()) {
      // Buffer does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }

    buffer* b = bpos->second;
    if (b->begin () != 0) {
      // Buffer was mapped.
      return make_pair (-1, LILY_SYSCALL_EMAPPED);
    }

    b->resize (size);
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  pair<size_t, int>
  buffer_append (bd_t dst,
		 bd_t src,
		 size_t begin,
		 size_t end)
  {
    if (begin > end) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }

    bd_to_buffer_map_type::const_iterator dst_pos = bd_to_buffer_map_.find (dst);
    bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
    if (dst_pos == bd_to_buffer_map_.end () ||
	src_pos == bd_to_buffer_map_.end ()) {
      // One of the buffers does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }

    buffer* d = dst_pos->second;
    buffer* s = src_pos->second;
    if (end > s->size ()) {
      // Offset is past end of source.
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }
    
    if (d->begin () != 0) {
      // The destination is mapped.
      return make_pair (-1, LILY_SYSCALL_EMAPPED);
    }

    // Append.
    return make_pair (d->append (*s, begin, end), LILY_SYSCALL_ESUCCESS);
  }

  int
  buffer_assign (bd_t dest,
		 size_t dest_begin,
		 bd_t src,
		 size_t src_begin,
		 size_t src_end)
  {
    if (src_begin > src_end) {
      // Bad range.
      return -1;
    }

    bd_to_buffer_map_type::const_iterator dest_pos = bd_to_buffer_map_.find (dest);
    bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
    if (dest_pos == bd_to_buffer_map_.end () ||
	src_pos == bd_to_buffer_map_.end ()) {
      // One of the buffers does not exist.
      return -1;
    }

    buffer* dest_b = dest_pos->second;
    buffer* src_b = src_pos->second;

    if (src_end > src_b->size ()) {
      return -1;
    }

    if (dest_begin + (src_end - src_begin) > dest_b->size ()) {
      return -1;
    }

    // Assign.
    dest_b->assign (dest_begin, *src_b, src_begin, src_end);
    return 0;
  }

  pair<void*, int>
  buffer_map (bd_t bd)
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);

    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos == bd_to_buffer_map_.end ()) {
      // The buffer does not exist.
      return make_pair ((void*)0, LILY_SYSCALL_EBDDNE);
    }

    buffer* b = bpos->second;
    
    if (b->size () == 0) {
      // The buffer is empty.
      return make_pair ((void*)0, LILY_SYSCALL_EEMPTY);
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
      if (size >= b->size () * PAGE_SIZE) {
	b->map_end ((*stack_pos)->begin ());
	memory_map_.insert (prev.base (), b);
	// Success.
	return make_pair (reinterpret_cast<void*> (b->begin ()), LILY_SYSCALL_ESUCCESS);
      }
    }
    
    // Couldn't find a big enough hole.
    return make_pair ((void*)0, LILY_SYSCALL_ENOMEM);
  }

  pair<int, int>
  buffer_unmap (bd_t bd)
  {
    bd_to_buffer_map_type::iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos == bd_to_buffer_map_.end ()) {
      // The buffer does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }
    
    buffer* b = bpos->second;
    
    if (b->begin () != 0) {
      // Remove from the memory map.
      memory_map_.erase (find_address (b->begin ()));
      
      // Unmap the buffer.	
      b->unmap ();
    }

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  buffer*
  buffer_output_destroy (bd_t bd)
  {
    bd_to_buffer_map_type::iterator bpos = bd_to_buffer_map_.find (bd);
    kassert (bpos != bd_to_buffer_map_.end ());

    buffer* b = bpos->second;

    if (b->begin () != 0) {
      // Remove from the memory map.
      memory_map_.erase (find_address (b->begin ()));
      
      // Unmap the buffer.
      b->unmap ();
    }
    
    // Remove from the bd map.
    bd_to_buffer_map_.erase (bpos);

    return b;
  }

  pair<int, int>
  buffer_destroy (bd_t bd)
  {
    bd_to_buffer_map_type::iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos != bd_to_buffer_map_.end ()) {
      buffer* b = bpos->second;

      if (b->begin () != 0) {
	// Remove from the memory map.
	memory_map_.erase (find_address (b->begin ()));

	// Unmap the buffer.	
	b->unmap ();
      }

      // Remove from the bd map.
      bd_to_buffer_map_.erase (bpos);

      // Destroy it.
      delete b;

      return make_pair (0, LILY_SYSCALL_ESUCCESS);
    }
    else {
      // The buffer does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }
  }

  pair<size_t, int>
  buffer_size (bd_t bd)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos != bd_to_buffer_map_.end ()) {
      return make_pair (bpos->second->size (), LILY_SYSCALL_ESUCCESS);
    }
    else {
      // The buffer does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }
  }

  bool
  buffer_exists (bd_t bd)
  {
    return bd_to_buffer_map_.find (bd) != bd_to_buffer_map_.end ();
  }

  buffer*
  lookup_buffer (bd_t bd)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos != bd_to_buffer_map_.end ()) {
      return bpos->second;
    }
    else {
      return 0;
    }
  }

};

#endif /* __automaton_hpp__ */
