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
#include "binding.hpp"
#include "bitset.hpp"
#include "mutex.hpp"
#include "lock.hpp"
#include "shared_ptr.hpp"

/*
  Thread Safety
  =============
  Protected by id_mutex_:
  * current_aid_
  * aid_to_automaton_map_
  * registry_map_

  Protected by mod_mutex_:
  * enabled_ (can read to optimize check as it only transitions from true to false)
  * ano_to_action_map_
  * parent_
  * children_

  In the create functions, the locking order is id_mutex_, parent->mod_mutex_, and then child->mod_mutex_.

 */

// The stack.
static const logical_address_t STACK_END = KERNEL_VIRTUAL_BASE;
static const logical_address_t STACK_BEGIN = STACK_END - PAGE_SIZE;

class automaton {

  /**********************************************************************
   * STATIC VARIABLES                                                   *
   **********************************************************************/

  /*
   * IDENTIFICATION
   */

private:
  // Next automaton id to allocate.
  static aid_t current_aid_;

  // Map from aid to automaton.
  typedef unordered_map<aid_t, shared_ptr<automaton> > aid_to_automaton_map_type;
  static aid_to_automaton_map_type aid_to_automaton_map_;

  // A registry that maps a string (name) to an aid.
  typedef unordered_map<kstring, shared_ptr<automaton>, kstring_hash> registry_map_type;
  static registry_map_type registry_map_;

  /*
   * DESCRIPTION
   */

  /*
   * AUTOMATA HIERARCHY
   */

  /*
   * EXECUTION
   */

  /*
   * MEMORY MAP AND BUFFERS
   */

  /*
   * BINDING
   */

  // Next binding id to allocate.
  static bid_t current_bid_;

  // Map from bid to binding.
  typedef unordered_map<bid_t, shared_ptr<binding> > bid_to_binding_map_type;
  static bid_to_binding_map_type bid_to_binding_map_;

  /*
   * I/O
   */

private:
  // Bitset marking which frames are used for memory-mapped I/O.
  // I assume the region from 0x0 to 0x00100000 is used for memory-mapped I/O.
  // Need one bit for each frame.
  static bitset<ONE_MEGABYTE / PAGE_SIZE> mmapped_frames_;

  // Bitset marking which I/O ports are reserved.
  static bitset<65536> reserved_ports_;

  /*
   * SUBSCRIPTIONS
   */

  /*
   * CREATION AND DESTRUCTION
   */

  /**********************************************************************
   * STATIC METHODS                                                     *
   **********************************************************************/

  /*
   * IDENTIFICATION
   */

  /*
   * DESCRIPTION
   */

  /*
   * AUTOMATA HIERARCHY
   */

public:
  static pair<shared_ptr<automaton>, int>
  create_automaton (const kstring& name,
		    const shared_ptr<automaton>& parent,
		    bool privileged,
		    buffer* text,
		    size_t text_size,
		    buffer* buffer_a,
		    buffer* buffer_b);

  /*
   * EXECUTION
   */

  /*
   * MEMORY MAP AND BUFFERS
   */

  /*
   * BINDING
   */

  /*
   * I/O
   */

  /*
   * SUBSCRIPTIONS
   */

  /*
   * CREATION AND DESTRUCTION
   */

  /**********************************************************************
   * INSTANCE VARIABLES                                                 *
   **********************************************************************/

  /*
   * IDENTIFICATION
   */

private:
  aid_t aid_;
  kstring name_;

  /*
   * DESCRIPTION
   */

  // Map from action number to action.
  typedef unordered_map<ano_t, const paction* const> ano_to_action_map_type;
  ano_to_action_map_type ano_to_action_map_;
  // Description of the actions.
  kstring description_;
  // Flag indicating the description should be regenerated from the list of actions.
  bool regenerate_description_;

  /*
   * AUTOMATA HIERARCHY
   */

private:
  shared_ptr<automaton> parent_;
public:
  typedef unordered_set<shared_ptr<automaton> > children_set_type;
private:
  children_set_type children_;

  /*
   * EXECUTION
   */

  // Mutual exlusion lock for executing actions.
  mutex execution_mutex_;

  /*
   * MEMORY MAP AND BUFFERS
   */

public:
  // Physical address that contains the automaton's page directory.
  physical_address_t const page_directory;
private:
  // Memory map. Consider using a set/map if insert/remove becomes too expensive.
  typedef vector<vm_area_base*> memory_map_type;
  memory_map_type memory_map_;
  // Heap area.
  vm_area_base* heap_area_;
  // Stack area.
  vm_area_base* stack_area_;
  // Next buffer descriptor to allocate.
  bd_t current_bd_;
  // Map from bd_t to buffer*.
  typedef unordered_map<bd_t, buffer*> bd_to_buffer_map_type;
  bd_to_buffer_map_type bd_to_buffer_map_;

  /*
   * BINDING
   */

  typedef unordered_set <shared_ptr<binding> > binding_set_type;
  // Bound outputs.
  typedef unordered_map <caction, binding_set_type, caction_hash> bound_outputs_map_type;
  bound_outputs_map_type bound_outputs_map_;
  // Bound inputs.
  typedef unordered_map<caction, binding_set_type, caction_hash> bound_inputs_map_type;
  bound_inputs_map_type bound_inputs_map_;
  // Owned bindings.
  binding_set_type owned_bindings_;

  /*
   * I/O
   */

  bool privileged_;
  // Memory mapped areas.
  typedef vector<mapped_area*> mapped_areas_type;
  mapped_areas_type mapped_areas_;
  // Reserved I/O ports.
  typedef unordered_set<unsigned short> port_set_type;
  port_set_type port_set_;
  // Interrupts.
  typedef unordered_map<int, caction> irq_map_type;
  irq_map_type irq_map_;

  /*
   * SUBSCRIPTIONS
   */

  // Automata to which this automaton is subscribed.
  typedef unordered_set<shared_ptr<automaton> > automaton_subscriptions_type;
  automaton_subscriptions_type automaton_subscriptions_;
  // Automata that receive notification when this automaton is destroyed.
  typedef unordered_set<shared_ptr<automaton> > subscribers_set_type;
  subscribers_set_type subscribers_;
  // Bindings to which this automaton is subscribed.
  typedef unordered_set<shared_ptr<binding> > binding_subscriptions_type;
  binding_subscriptions_type binding_subscriptions_;

  /*
   * CREATION AND DESTRUCTION
   */

  /**********************************************************************
   * INSTANCE METHODS                                                   *
   **********************************************************************/

  /*
   * IDENTIFICATION
   */

public:
  inline aid_t
  aid () const
  {
    return aid_;
  }

  inline pair<aid_t, int>
  lookup (const char* name,
	  size_t size)
  {
    // Check the name.
    if (!verify_span (name, size) || name[size -1] != 0) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }

    // Check the name in the map.
    const kstring n (name, size);

    registry_map_type::const_iterator pos = registry_map_.find (n);
    if (pos != registry_map_.end ()) {
      return make_pair (pos->second->aid (), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_ESUCCESS);
    }
  }

  /*
   * DESCRIPTION
   */

public:
  inline bool
  add_action (action_type_t t,
	      parameter_mode_t pm,
	      const void* aep,
	      ano_t an,
	      const kstring& name,
	      const kstring& description)
  {
    // TODO:  What if the actions have the same name?
    if (ano_to_action_map_.find (an) == ano_to_action_map_.end ()) {
      paction* action = new paction (t, pm, aep, an, name, description);
      ano_to_action_map_.insert (make_pair (an, action));
      regenerate_description_ = true;
      return true;
    }
    else {
      return false;
    }
  }

  inline const paction*
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
  
  inline pair<bd_t, int>
  describe (aid_t aid)
  {
    aid_to_automaton_map_type::const_iterator pos = aid_to_automaton_map_.find (aid);
    if (pos == aid_to_automaton_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EAIDDNE);
    }

    shared_ptr<automaton> subject = pos->second;

    if (subject->regenerate_description_) {
      // Form a description of the actions.
      subject->description_.clear ();

      size_t action_count = subject->ano_to_action_map_.size ();
      subject->description_.append (&action_count, sizeof (action_count));
  
      for (ano_to_action_map_type::const_iterator pos = subject->ano_to_action_map_.begin ();
	   pos != subject->ano_to_action_map_.end ();
	   ++pos) {
	subject->description_.append (&pos->second->type, sizeof (action_type_t));
	subject->description_.append (&pos->second->parameter_mode, sizeof (parameter_mode_t));
	subject->description_.append (&pos->second->action_number, sizeof (ano_t));
	size_t size = pos->second->name.size ();
	subject->description_.append (&size, sizeof (size_t));
	size = pos->second->description.size ();
	subject->description_.append (&size, sizeof (size_t));
	subject->description_.append (pos->second->name.c_str (), pos->second->name.size ());
	subject->description_.append (pos->second->description.c_str (), pos->second->description.size ());
      }
    }

    const kstring& desc = subject->description_;
    const size_t total_size = sizeof (size_t) + desc.size ();

    // Create a buffer for the description.
    // We create an empty buffer and then manually add frames to it.
    // This avoids using the zero_frame which is important because the copy-on-write system won't be triggered since we are in supervisor mode.
    pair<bd_t, int> r = buffer_create (0);
    if (r.first == -1) {
      return r;
    }
    buffer* b = lookup_buffer (r.first);
    size_t page_count = align_up (total_size, PAGE_SIZE) / PAGE_SIZE;
    for (size_t idx = 0; idx != page_count; ++idx) {
      frame_t frame = frame_manager::alloc ();
      kassert (frame != vm::zero_frame ());
      b->append_frame (frame);
      /* Drop the reference count. */
      frame_manager::decref (frame);
    }

    pair<void*, int> s = buffer_map (r.first);
    if (s.first == 0) {
      // Destroy the buffer.
      buffer_destroy (r.first);
      return make_pair (-1, s.second);
    }

    memcpy (s.first, &total_size, sizeof (size_t));
    memcpy (reinterpret_cast<char*> (s.first) + sizeof (size_t), desc.c_str (), desc.size ());
    memset (reinterpret_cast<char*> (s.first) + total_size, 0, page_count * PAGE_SIZE - total_size);

    return make_pair (r.first, LILY_SYSCALL_ESUCCESS);
  }

  /*
   * AUTOMATA HIERARCHY
   */
  
  pair<aid_t, int>
  create (const shared_ptr<automaton>& ths,
	  bd_t text_bd,
	  size_t /*text_size*/,
	  bd_t bda,
	  bd_t bdb,
	  const char* name,
	  size_t name_size,
	  bool retain_privilege);

  pair<int, int>
  destroy (aid_t aid)
  {
    // BUG
    kassert (0);
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  /*
   * EXECUTION
   */

public:
  inline void
  lock_execution ()
  {
    execution_mutex_.lock ();
  }
  
  // Does not return.
  inline void
  execute (const paction& action,
	   int parameter,
	   buffer* output_buffer_a_,
	   buffer* output_buffer_b_)
  {
    // switch (action_.action->type) {
    // case INPUT:
    // 	kout << "?";
    // 	break;
    // case OUTPUT:
    // 	kout << "!";
    // 	break;
    // case INTERNAL:
    // 	kout << "#";
    // 	break;
    // case SYSTEM_INPUT:
    // 	kout << "*";
    // 	break;
    // }
    // kout << "\t" << action_.action->automaton->aid () << "\t" << action_.action->action_number << "\t" << action_.parameter << endl;
    
    // Switch page directories.
    vm::switch_to_directory (page_directory);
    
    uint32_t* stack_pointer = reinterpret_cast<uint32_t*> (stack_area_->end ());
    
    // These instructions serve a dual purpose.
    // First, they set up the cdecl calling convention for actions.
    // Second, they force the stack to be created if it is not.
    
    switch (action.type) {
    case INPUT:
    case SYSTEM_INPUT:
      {
	bd_t bda = -1;
	bd_t bdb = -1;
	
	if (output_buffer_a_ != 0) {
	  // Copy the buffer to the input automaton.
	  bda = buffer_create (*output_buffer_a_);
	}
	
	if (output_buffer_b_ != 0) {
	  // Copy the buffer to the input automaton.
	  bdb = buffer_create (*output_buffer_b_);
	}
	
	// Push the buffers.
	*--stack_pointer = bdb;
	*--stack_pointer = bda;
      }
      break;
    case OUTPUT:
    case INTERNAL:
      // Do nothing.
      break;
    }
    
    // Push the parameter.
    *--stack_pointer = parameter;
    
    // Push a bogus instruction pointer so we can use the cdecl calling convention.
    *--stack_pointer = 0;
    uint32_t* sp = stack_pointer;
    
    // Push the stack segment.
    *--stack_pointer = gdt::USER_DATA_SELECTOR | descriptor::RING3;
    
    // Push the stack pointer.
    *--stack_pointer = reinterpret_cast<uint32_t> (sp);
    
    // Push the flags.
    uint32_t eflags;
    asm ("pushf\n"
	 "pop %0\n" : "=g"(eflags) : :);
    *--stack_pointer = eflags;
    
    // Push the code segment.
    *--stack_pointer = gdt::USER_CODE_SELECTOR | descriptor::RING3;
    
    // Push the instruction pointer.
    *--stack_pointer = reinterpret_cast<uint32_t> (action.action_entry_point);
    
    asm ("mov %0, %%ds\n"	// Load the data segments.
	 "mov %0, %%es\n"	// Load the data segments.
	 "mov %0, %%fs\n"	// Load the data segments.
	 "mov %0, %%gs\n"	// Load the data segments.
	 "mov %1, %%esp\n"	// Load the new stack pointer.
	 "xor %%eax, %%eax\n"	// Clear the registers.
	 "xor %%ebx, %%ebx\n"
	 "xor %%ecx, %%ecx\n"
	 "xor %%edx, %%edx\n"
	 "xor %%edi, %%edi\n"
	 "xor %%esi, %%esi\n"
	 "xor %%ebp, %%ebp\n"
	 "iret\n" :: "r"(gdt::USER_DATA_SELECTOR | descriptor::RING3), "r"(stack_pointer));
  }

  // The automaton has finished the current action.
  //
  // action_number and parameter can be used to schedule another action.
  // These are ignored if the action does not exist.
  //
  // output_fired indicates that an output action fired.
  // bda and bdb are buffers that will be be passed to input actions from an output action.
  // These are passed to the scheduler which ignores them if the action that called finish is not an output action.
  void
  finish (const shared_ptr<automaton>& ths,
	  ano_t action_number,
  	  int parameter,
  	  bool output_fired,
  	  bd_t bda,
  	  bd_t bdb);

  // The automaton would like to no longer exist.
  inline void
  exit (void)
  {
    // BUG
    kassert (0);

    // // Disable the automaton.
    // disable ();

    // // Tell the automaton and its children to forget their parents.
    // automaton* parent = forget_parent ();

    // // Remove all of the bindings.
    // purge_bindings ();

    // // Remove from the scheduler.
    // remove_from_scheduler (a);

    // // Forget all of the children.
    // forget_children ();

    // // Tell the parent to forget its child.
    // if (parent != 0) {
    //   parent->forget_child (a);
    // }

    // scheduler::finish (false, -1, -1);
  }

  inline void
  unlock_execution ()
  {
    execution_mutex_.unlock ();
  }

  /*
   * MEMORY MAP AND BUFFERS
   */

private:
  struct compare_vm_area {
    inline bool
    operator () (const vm_area_base* const x,
		 const vm_area_base* const y) const
    {
      return x->begin () < y->begin ();
    }
  };

  struct contains_address {
    logical_address_t address;

    contains_address (const void* ptr) :
      address (reinterpret_cast<logical_address_t> (ptr))
    { }

    inline bool
    operator() (const mapped_area* area)
    {
      return address >= area->begin () && address < area->end ();
    }
  };

  inline memory_map_type::const_iterator
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

  inline memory_map_type::iterator
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

  inline bd_t
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

public:
  inline bool
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
  
  inline void
  map_heap_and_stack ()
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);
    kassert (page_directory == vm::get_directory ());

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
      vm::map (address, frame, vm::USER, vm::MAP_READ_WRITE);
    }
  }

  inline bool
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

  inline void
  insert_vm_area (vm_area_base* area)
  {
    kassert (area != 0);
    
    // Find the location to insert.
    memory_map_type::iterator pos = upper_bound (memory_map_.begin (), memory_map_.end (), area, compare_vm_area ());
    memory_map_.insert (pos, area);
  }

private:
  inline bool
  verify_span (const void* ptr,
  	       size_t size) const
  {
    const logical_address_t address = reinterpret_cast<logical_address_t> (ptr);
    memory_map_type::const_iterator pos = find_address (address);
    return pos != memory_map_.end () && (*pos)->begin () <= address && (address + size) <= (*pos)->end ();
  }

public:
  inline bool
  verify_stack (const void* ptr,
		size_t size) const
  {
    
    kassert (stack_area_ != 0);
    const logical_address_t address = reinterpret_cast<logical_address_t> (ptr);
    return stack_area_->begin () <= address && (address + size) <= stack_area_->end ();
  }

  inline pair<void*, int>
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
	    vm::map (address, vm::zero_frame (), vm::USER, vm::MAP_COPY_ON_WRITE);
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

  // TODO:  Limit the number and size of buffers.

  inline pair<bd_t, int>
  buffer_create (size_t size)
  {
    
    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new buffer (size);
    bd_to_buffer_map_.insert (make_pair (bd, b));

    return make_pair (bd, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<bd_t, int>
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
  
  inline bd_t
  buffer_create (const buffer& other)
  {
    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new buffer (other);
    bd_to_buffer_map_.insert (make_pair (bd, b));
    
    return bd;
  }

  inline pair<size_t, int>
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

  inline pair<size_t, int>
  buffer_append (bd_t dst,
		 bd_t src)
  {
    bd_to_buffer_map_type::const_iterator dst_pos = bd_to_buffer_map_.find (dst);
    bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
    if (dst_pos == bd_to_buffer_map_.end () ||
	src_pos == bd_to_buffer_map_.end ()) {
      // One of the buffers does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }

    buffer* d = dst_pos->second;
    buffer* s = src_pos->second;
    
    if (d->begin () != 0) {
      // The destination is mapped.
      return make_pair (-1, LILY_SYSCALL_EMAPPED);
    }

    // Append.
    return make_pair (d->append (*s, 0, s->size ()), LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
  buffer_assign (bd_t dest,
		 bd_t src)
  {
    bd_to_buffer_map_type::const_iterator dest_pos = bd_to_buffer_map_.find (dest);
    bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
    if (dest_pos == bd_to_buffer_map_.end () ||
	src_pos == bd_to_buffer_map_.end ()) {
      // One of the buffers does not exist.
      return make_pair (-1, LILY_SYSCALL_EBDDNE);
    }

    buffer* dest_b = dest_pos->second;
    buffer* src_b = src_pos->second;

    // Truncate and append.
    dest_b->resize (0);
    dest_b->append (*src_b, 0, src_b->size ());

    // Append.
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<void*, int>
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

  inline pair<int, int>
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

  inline pair<int, int>
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

  inline pair<size_t, int>
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

  inline buffer*
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

  /*
   * BINDING
   */

  /******************************************************************************************
   * The functions between lock_bindings and unlock_bindings all require the bindings lock. *
   ******************************************************************************************/

  template <typename OutputIterator>
  inline void
  copy_bound_inputs (const caction& output_action,
		     OutputIterator iter) const
  {
    bound_outputs_map_type::const_iterator pos = bound_outputs_map_.find (output_action);
    if (pos != bound_outputs_map_.end ()) {
      copy (pos->second.begin (), pos->second.end (), iter);
    }
  }

  inline pair<bid_t, int>
  bind (const shared_ptr<automaton>& ths,
	aid_t output_aid,
	ano_t output_ano,
	int output_parameter,
	aid_t input_aid,
	ano_t input_ano,
	int input_parameter)
  {
    kassert (ths.get () == this);
    
    aid_to_automaton_map_type::const_iterator output_pos = aid_to_automaton_map_.find (output_aid);
    if (output_pos == aid_to_automaton_map_.end ()) {
      // Output automaton DNE.
      return make_pair (-1, LILY_SYSCALL_EOAIDDNE);
    }
    
    aid_to_automaton_map_type::const_iterator input_pos = aid_to_automaton_map_.find (input_aid);
    if (input_pos == aid_to_automaton_map_.end ()) {
      // Input automaton DNE.
      return make_pair (-1, LILY_SYSCALL_EIAIDDNE);
    }
    
    shared_ptr<automaton> output_automaton = output_pos->second;
    
    shared_ptr<automaton> input_automaton = input_pos->second;
    
    if (output_automaton == input_automaton) {
      // The output and input automata must be different.
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }
    
    // Check the output action dynamically.
    const paction* output_action = output_automaton->find_action (output_ano);
    if (output_action == 0 ||
	output_action->type != OUTPUT) {
      // Output action does not exist or has the wrong type.
      return make_pair (-1, LILY_SYSCALL_EOBADANO);
    }
    
    // Check the input action dynamically.
    const paction* input_action = input_automaton->find_action (input_ano);
    if (input_action == 0 ||
	input_action->type != INPUT) {
      // Input action does not exist or has the wrong type.
      return make_pair (-1, LILY_SYSCALL_EIBADANO);
    }
    
    // Correct the parameters.
    switch (output_action->parameter_mode) {
    case NO_PARAMETER:
      output_parameter = 0;
      break;
    case PARAMETER:
      break;
    case AUTO_PARAMETER:
      output_parameter = input_automaton->aid ();
      break;
    }
    
    switch (input_action->parameter_mode) {
    case NO_PARAMETER:
      input_parameter = 0;
      break;
    case PARAMETER:
      break;
    case AUTO_PARAMETER:
      input_parameter = output_automaton->aid ();
      break;
    }
    
    // Form complete actions.
    caction oa (output_automaton, output_action, output_parameter);
    caction ia (input_automaton, input_action, input_parameter);
    
    // Check that the input action is not bound.
    // (Also checks that the binding does not exist.)
    if (input_automaton->bound_inputs_map_.find (ia) != input_automaton->bound_inputs_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EINVAL);
    }
    
    // Check that the output action is not bound to an action in the input automaton.
    bound_outputs_map_type::const_iterator pos1 = output_automaton->bound_outputs_map_.find (oa);
    if (pos1 != output_automaton->bound_outputs_map_.end ()) {
      for (binding_set_type::const_iterator pos2 = pos1->second.begin (); pos2 != pos1->second.end (); ++pos2) {
	if ((*pos2)->input_action.automaton == input_automaton) {
	  return make_pair (-1, LILY_SYSCALL_EINVAL);
	}
      }
    }
    
    // Generate an id.
    bid_t bid = current_bid_;
    while (bid_to_binding_map_.find (bid) != bid_to_binding_map_.end ()) {
      bid = max (bid + 1, 0); // Handles overflow.
    }
    current_bid_ = max (bid + 1, 0);
    
    // Create the binding.
    shared_ptr<binding> b = shared_ptr<binding> (new binding (bid, oa, ia, ths));
    bid_to_binding_map_.insert (make_pair (bid, b));
    
    // Bind.
    {
      pair<bound_outputs_map_type::iterator, bool> r = output_automaton->bound_outputs_map_.insert (make_pair (oa, binding_set_type ()));
      r.first->second.insert (b);
    }
    
    {
      pair<bound_inputs_map_type::iterator, bool> r = input_automaton->bound_inputs_map_.insert (make_pair (ia, binding_set_type ()));
      r.first->second.insert (b);
    }
    
    {
      pair <binding_set_type::iterator, bool> r = owned_bindings_.insert (b);
      kassert (r.second);
    }
    
    return make_pair (b->bid, LILY_SYSCALL_ESUCCESS);
  }

  pair<int, int>
  unbind (bid_t bid)
  {
    // BUG
    kassert (0);
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  /*
   * I/O
   */

public:
  // The automaton is requesting that the physical memory from [source, source + size) appear at [destination, destination + size) in its address space.
  // The destination and source are aligned to a page boundary and the size is rounded up to a multiple of the page size.
  // Map can fail for the following reasons:
  // *  The automaton is not privileged.
  // *  The low order bits of the destination and source do not agree when interpretted using the page size.
  // *  Size is zero.
  // *  Part of the source lies outside the regions of memory marked for memory-mapped I/O.
  // *  Part of the source is already claimed by some other automaton.  (Mapping the same region twice is an error.)
  // *  The destination address is not available.
  inline pair<int, int>
  map (const void* destination,
       const void* source,
       size_t size)
  {
    logical_address_t const destination_begin = align_down (reinterpret_cast<logical_address_t> (destination), PAGE_SIZE);
    logical_address_t const destination_end = align_up (reinterpret_cast<logical_address_t> (destination) + size, PAGE_SIZE);
    physical_address_t const source_begin = align_down (reinterpret_cast<physical_address_t> (source), PAGE_SIZE);
    physical_address_t const source_end = align_up (reinterpret_cast<physical_address_t> (source) + size, PAGE_SIZE);

    if (!privileged_) {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }

    if ((reinterpret_cast<logical_address_t> (destination) & (PAGE_SIZE - 1)) != (reinterpret_cast<physical_address_t> (source) & (PAGE_SIZE - 1))) {
      return make_pair (-1, LILY_SYSCALL_EALIGN);
    }

    if (size == 0) {
      return make_pair (-1, LILY_SYSCALL_ESIZE);
    }

    // I assume that all memory mapped I/O involves the region between 0 and 0x00100000.
    if (source_end > ONE_MEGABYTE) {
      return make_pair (-1, LILY_SYSCALL_ESRCNA);
    }

    for (physical_address_t address = source_begin; address != source_end; address += PAGE_SIZE) {
      if (mmapped_frames_[physical_address_to_frame (address)]) {
  	return make_pair (-1, LILY_SYSCALL_ESRCINUSE);
      }
    }

    if (!vm_area_is_free (destination_begin, destination_end)) {
      return make_pair (-1, LILY_SYSCALL_EDSTINUSE);
    }
    
    mapped_area* area = new mapped_area (destination_begin,
  					 destination_end,
  					 source_begin,
  					 source_end);

    insert_vm_area (area);
    mapped_areas_.push_back (area);

    physical_address_t pa = source_begin;
    for (logical_address_t la = destination_begin; la != destination_end; la += PAGE_SIZE, pa += PAGE_SIZE) {
      mmapped_frames_[physical_address_to_frame (pa)] = true;
      vm::map (la, physical_address_to_frame (pa), vm::USER, vm::MAP_READ_WRITE, false);
    }

    // Success.
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
  unmap (const void* destination)
  {
    mapped_areas_type::iterator pos = find_if (mapped_areas_.begin (), mapped_areas_.end (), contains_address (destination));
    if (pos == mapped_areas_.end ()) {
      return make_pair (-1, LILY_SYSCALL_ENOTMAPPED);
    }

    mapped_area* area = *pos;

    physical_address_t pa = area->physical_begin;
    for (logical_address_t la = area->begin ();
	 la != area->end ();
	 la += PAGE_SIZE, pa += PAGE_SIZE) {
      vm::unmap (la);
      mmapped_frames_[physical_address_to_frame (pa)] = false;
    }

    mapped_areas_.erase (pos);

    memory_map_type::iterator pos2 = find (memory_map_.begin (), memory_map_.end (), area);
    kassert (pos2 != memory_map_.end ());
    memory_map_.erase (pos2);
    
    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  // The automaton is requesting access to the specified I/O port.
  inline pair<int, int>
  reserve_port (unsigned short port)
  {
    if (!privileged_) {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }

    if (reserved_ports_[port]) {
      // Port is already reserved.
      return make_pair (-1, LILY_SYSCALL_EPORTINUSE);
    }

    // Reserve the port.
    reserved_ports_[port] = true;
    port_set_.insert (port);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
  unreserve_port (unsigned short port)
  {
    port_set_type::iterator pos = port_set_.find (port);
    if (pos == port_set_.end ()) {
      return make_pair (-1, LILY_SYSCALL_ENOTRESERVED);
    }

    reserved_ports_[port] = false;
    port_set_.erase (pos);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<uint8_t, int>
  inb (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inb (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  inline pair<int, int>
  outb (uint16_t port,
	uint8_t value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outb (port, value);
      return make_pair (0, LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  inline pair<uint16_t, int>
  inw (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inw (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  inline pair<int, int>
  outw (uint16_t port,
	uint16_t value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outw (port, value);
      return make_pair (0, LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  inline pair<uint32_t, int>
  inl (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inl (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  inline pair<int, int>
  outl (uint16_t port,
	uint32_t value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outl (port, value);
      return make_pair (0, LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  inline pair<int, int>
  subscribe_irq (const shared_ptr<automaton>& ths,
		 int irq,
		 ano_t action_number,
		 int parameter)
  {
    kassert (ths.get () == this);

    if (!privileged_) {
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
      /* Already subscribed.  Note that we don't check the action. */
      return make_pair (-1, LILY_SYSCALL_EALREADY);
    }

    caction c (ths, action, parameter);

    irq_map_.insert (make_pair (irq, c));
    irq_handler::subscribe (irq, c);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
  unsubscribe_irq (int irq)
  {
    irq_map_type::iterator pos = irq_map_.find (irq);
    if (pos == irq_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_ENOTSUBSCRIBED);
    }

    irq_handler::unsubscribe (pos->first, pos->second);
    irq_map_.erase (pos);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
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

  /*
   * SUBSCRIPTIONS
   */

  inline pair<int, int>
  subscribe_unbound (const shared_ptr<automaton>& ths,
		     bid_t bid,
		     ano_t action_number)
  {
    kassert (ths.get () == this);

    const paction* action = find_action (action_number);
    if (action == 0 || action->type != SYSTEM_INPUT || action->parameter_mode != PARAMETER) {
      return make_pair (-1, LILY_SYSCALL_EBADANO);
    }

    bid_to_binding_map_type::const_iterator subject_pos = bid_to_binding_map_.find (bid);
    if (subject_pos == bid_to_binding_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EBIDDNE);
    }

    shared_ptr<binding> subject = subject_pos->second;

    binding_subscriptions_.insert (subject);
    subject->subscribers.insert (ths);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
  unsubscribe_unbound (const shared_ptr<automaton>& ths,
		       bid_t bid)
  {
    bid_to_binding_map_type::const_iterator subject_pos = bid_to_binding_map_.find (bid);
    if (subject_pos == bid_to_binding_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EBIDDNE);
    }

    shared_ptr<binding> subject = subject_pos->second;

    size_t count = binding_subscriptions_.erase (subject);
    if (count == 0) {
      return make_pair (-1, LILY_SYSCALL_ENOTSUBSCRIBED);
    }

    count = subject->subscribers.erase (ths);
    kassert (count == 1);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  // The automaton wants to receive a notification via action_number when the automaton corresponding to aid is destroyed.
  // The automaton must have an action for receiving the event.
  // The automaton corresponding to aid must exist.
  // Not an error if already subscribed.
  inline pair<int, int>
  subscribe_destroyed (const shared_ptr<automaton>& ths,
		       aid_t aid,
		       ano_t action_number)
  {
    kassert (ths.get () == this);

    const paction* action = find_action (action_number);
    if (action == 0 || action->type != SYSTEM_INPUT || action->parameter_mode != PARAMETER) {
      return make_pair (-1, LILY_SYSCALL_EBADANO);
    }

    aid_to_automaton_map_type::const_iterator subject_pos = aid_to_automaton_map_.find (aid);
    if (subject_pos == aid_to_automaton_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EAIDDNE);
    }

    shared_ptr<automaton> subject = subject_pos->second;

    automaton_subscriptions_.insert (subject);
    subject->subscribers_.insert (ths);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  inline pair<int, int>
  unsubscribe_destroyed (const shared_ptr<automaton>& ths,
			 aid_t aid)
  {
    aid_to_automaton_map_type::const_iterator subject_pos = aid_to_automaton_map_.find (aid);
    if (subject_pos == aid_to_automaton_map_.end ()) {
      return make_pair (-1, LILY_SYSCALL_EAIDDNE);
    }

    shared_ptr<automaton> subject = subject_pos->second;

    size_t count = automaton_subscriptions_.erase (subject);
    if (count == 0) {
      return make_pair (-1, LILY_SYSCALL_ENOTSUBSCRIBED);
    }

    count = subject->subscribers_.erase (ths);
    kassert (count == 1);

    return make_pair (0, LILY_SYSCALL_ESUCCESS);
  }

  /*
   * CREATION AND DESTRUCTION
   */

  automaton () :
    aid_ (-1),
    regenerate_description_ (false),
    parent_ (0),
    page_directory (frame_to_physical_address (frame_manager::alloc ())),
    heap_area_ (0),
    stack_area_ (0),
    current_bd_ (0),
    privileged_ (false)
  {
    frame_t frame = physical_address_to_frame (page_directory);
    kassert (frame != vm::zero_frame ());
    // Map the page directory.
    vm::map (vm::get_stub1 (), frame, vm::USER, vm::MAP_READ_WRITE);
    vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
    // Initialize the page directory with a copy of the kernel.
    // Since the second argument is vm::SUPERVISOR, the automaton cannot access the paging area, i.e., manipulate virtual memory.
    // If the third argument is vm::USER, the automaton gains access to kernel data.
    // If the third argument is vm::SUPERVISOR, the automaton does not gain access to kernel data.
    new (pd) vm::page_directory (frame, vm::SUPERVISOR, vm::SUPERVISOR);
    // Unmap.
    vm::unmap (vm::get_stub1 ());

    // At this point the page directory has a reference count of two.
    // One reference came from allocation.
    // The other reference came when we initialized the page directory (it refers to itself).
    kassert (frame_manager::ref_count (frame) == 2);
  }

private:
  // No copy.
  automaton (const automaton&);

public:
  ~automaton ()
  {
    // BUG
    kassert (0);

    // /*
    //  * IDENTIFICATION
    //  */

    // size_t count = aid_to_automaton_map_.erase (aid_);
    // kassert (count == 1);

    // /*
    //  * DESCRIPTION
    //  */

    // for (ano_to_action_map_type::const_iterator pos = ano_to_action_map_.begin ();
    // 	 pos != ano_to_action_map_.end ();
    // 	 ++pos) {
    //   delete pos->second;
    // }
    
    // /*
    //  * AUTOMATA HIERARCHY
    //  */
    
    // // kassert (parent_ == 0);
    // kassert (children_.empty ());
    
    // /*
    //  * EXECUTION
    //  */

    // //kassert (!enabled_);
    
    // /*
    //  * MEMORY MAP AND BUFFERS
    //  */
    
    // count = frame_manager::decref (physical_address_to_frame (page_directory));
    // kassert (count == 1);
    // count = frame_manager::decref (physical_address_to_frame (page_directory));
    // kassert (count == 0);

    // /*
    //  * BINDING
    //  */

    // kassert (bound_outputs_map_.empty ());
    // kassert (bound_inputs_map_.empty ());
    // kassert (owned_bindings_.empty ());
    
    // /*
    //  * I/O
    //  */

    // for (mapped_areas_type::const_iterator pos = mapped_areas_.begin ();
    // 	 pos != mapped_areas_.end ();
    // 	 ++pos) {
    //   mapped_area* area = *pos;
    //   memory_map_.erase (find (memory_map_.begin (), memory_map_.end (), area));

    //   for (physical_address_t address = area->physical_begin;
    // 	   address != area->physical_end;
    // 	   address += PAGE_SIZE) {
    // 	mmapped_frames_[physical_address_to_frame (address)] = false;
    //   }

    //   delete area;
    // }

    // for (port_set_type::const_iterator pos = port_set_.begin ();
    // 	 pos != port_set_.end ();
    // 	 ++pos) {
    //   reserved_ports_[*pos] = false;
    // }

    // for (irq_map_type::const_iterator pos = irq_map_.begin ();
    // 	 pos != irq_map_.end ();
    // 	 ++pos) {
    //   irq_handler::unsubscribe (pos->first, pos->second);
    // }
    
    // /*
    //  * SUBSCRIPTIONS
    //  */
    
    // /*
    //  * CREATION AND DESTRUCTION
    //  */
  }

};

#endif /* __automaton_hpp__ */
