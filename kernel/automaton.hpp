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
  // Memory mapped areas.
  typedef vector<mapped_area*> mapped_areas_type;
  static mapped_areas_type all_mapped_areas_;

  // Bitset marking which I/O ports are reserved.
  static bitset<65536> reserved_ports_;

  /*
   * SUBSCRIPTIONS
   */
  // Automata subscribing to log events.
  typedef unordered_map<shared_ptr<automaton>, const paction*> log_event_map_type;
  static log_event_map_type log_event_map_;
  // Automata subscribing to system events.
  typedef unordered_map<shared_ptr<automaton>, const paction*> system_event_map_type;
  static system_event_map_type system_event_map_;

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
  static pair<shared_ptr<automaton>, lily_error_t>
  create_automaton (const shared_ptr<automaton>& parent,
		    bool privileged,
		    const shared_ptr<buffer>& text,
		    size_t text_size,
		    const shared_ptr<buffer>& buffer_a,
		    const shared_ptr<buffer>& buffer_b);

  /*
   * EXECUTION
   */

  /*
   * MEMORY MAP AND BUFFERS
   */

  /*
   * BINDING
   */

private:
  static void
  unbind (const shared_ptr<binding>& binding,
	  bool remove_from_output,
	  bool remove_from_input,
	  bool remove_from_owner);

  /*
   * I/O
   */
public:
  static void
  reserve_port_s (unsigned short port)
  {
    reserved_ports_[port] = true;
  }

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

  /*
   * DESCRIPTION
   */

  // Map from action number to action.
  typedef unordered_map<ano_t, const paction* const> ano_to_action_map_type;
  ano_to_action_map_type ano_to_action_map_;
  // Map from action name to action.
  typedef unordered_map<kstring, const paction* const, kstring_hash> name_to_action_map_type;
  name_to_action_map_type name_to_action_map_;
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

  // Initialization buffers.
  bd_t init_buffer_a_;
  bd_t init_buffer_b_;

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
  typedef unordered_map<bd_t, shared_ptr<buffer> > bd_to_buffer_map_type;
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
    if (ano_to_action_map_.find (an) == ano_to_action_map_.end () &&
	(name.size () == 1 || name_to_action_map_.find (name) == name_to_action_map_.end ())) {
      paction* action = new paction (t, pm, aep, an, name, description);
      ano_to_action_map_.insert (make_pair (an, action));
      if (name.size () > 1) {
	name_to_action_map_.insert (make_pair (name, action));
      }
      regenerate_description_ = true;
      return true;
    }
    else {
      return false;
    }
  }

private:
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

public:  
  inline pair<bd_t, lily_error_t>
  describe (aid_t aid)
  {
    aid_to_automaton_map_type::const_iterator pos = aid_to_automaton_map_.find (aid);
    if (pos == aid_to_automaton_map_.end ()) {
      return make_pair (-1, LILY_ERROR_AIDDNE);
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
    size_t page_count = align_up (total_size, PAGE_SIZE) / PAGE_SIZE;

    // Create a buffer for the description.
    pair<bd_t, lily_error_t> r = buffer_create (page_count);
    if (r.first == -1) {
      return r;
    }

    pair<void*, lily_error_t> s = buffer_map (r.first);
    if (s.first == 0) {
      // Destroy the buffer.
      buffer_destroy (r.first);
      return make_pair (-1, s.second);
    }

    memcpy (s.first, &total_size, sizeof (size_t));
    memcpy (reinterpret_cast<char*> (s.first) + sizeof (size_t), desc.c_str (), desc.size ());

    return make_pair (r.first, LILY_ERROR_SUCCESS);
  }

  /*
   * AUTOMATA HIERARCHY
   */
  
  inline pair<aid_t, lily_error_t>
  create (const shared_ptr<automaton>& ths,
	  bd_t text_bd,
	  bd_t bda,
	  bd_t bdb,
	  bool retain_privilege)
  {
    kassert (ths.get () == this);
    
    // Find the text buffer.
    shared_ptr<buffer> text_buffer = lookup_buffer (text_bd);
    if (text_buffer.get () == 0) {
      // Buffer does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }
    
    // Find and synchronize the data buffers so the frames listed in the buffers are correct.
    shared_ptr<buffer> buffer_a = lookup_buffer (bda);
    if (buffer_a.get () != 0) {
      buffer_a->sync (0, buffer_a->size ());
    }
    shared_ptr<buffer> buffer_b = lookup_buffer (bdb);
    if (buffer_b.get () != 0) {
      buffer_b->sync (0, buffer_b->size ());
    }
    
    // Create the automaton.
    pair<shared_ptr<automaton>, lily_error_t> r = create_automaton (ths, retain_privilege && privileged_, text_buffer, text_buffer->size () * PAGE_SIZE, buffer_a, buffer_b);
    
    if (r.first.get () != 0) {
      kout << "TODO:  Generate create event" << endl;
      return make_pair (r.first->aid (), r.second);
    }
    else {
      return make_pair (-1, r.second);
    }
  }
  
private:
  void
  destroy (const shared_ptr<automaton>& ths);

public:
  inline pair<int, lily_error_t>
  destroy (const shared_ptr<automaton>& ths,
	   aid_t aid)
  {
    aid_to_automaton_map_type::iterator pos = aid_to_automaton_map_.find (aid);
    if (pos == aid_to_automaton_map_.end ()) {
      return make_pair (-1, LILY_ERROR_AIDDNE);
    }

    shared_ptr<automaton> child = pos->second;

    // Are we the parent?
    if (child->parent_ != ths) {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }

    child->destroy (child);

    size_t count = children_.erase (child);
    kassert (count == 1);

    return make_pair (0, LILY_ERROR_SUCCESS);
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
	   const shared_ptr<buffer>& output_buffer_a_,
	   const shared_ptr<buffer>& output_buffer_b_,
	   size_t binding_count)
  {
    // switch (action.type) {
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
    // kout << " ";
    // if (name_.c_str () != 0) {
    //   kout << name_.c_str ();
    // }
    // else {
    //   kout << aid_;
    // }
    // kout << "\t" << action.name.c_str () << "(" << action.action_number << ")" << "\t" << parameter << endl;
    
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
	
	if (output_buffer_a_.get () != 0) {
	  // Copy the buffer to the input automaton.
	  bda = buffer_create (output_buffer_a_);
	}
	
	if (output_buffer_b_.get () != 0) {
	  // Copy the buffer to the input automaton.
	  bdb = buffer_create (output_buffer_b_);
	}
	
	// Push the buffers.
	*--stack_pointer = bdb;
	*--stack_pointer = bda;
      }
      break;
    case OUTPUT:
      // Push the binding count.
      *--stack_pointer = binding_count;
      break;
    case INTERNAL:
      // Do nothing.
      break;
    }
    



    // Push the parameter.
    *--stack_pointer = parameter;

    // Push the action number.
    *--stack_pointer = action.action_number;
    
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
    
    //kout << "aid = " << aid_ << " ip = " << hexformat (action.action_entry_point) << endl;

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

  pair<int, lily_error_t>
  schedule (const shared_ptr<automaton>& ths,
	    ano_t action_number,
	    int parameter);

  // The automaton would like to no longer exist.
  void
  exit (const shared_ptr<automaton>& ths,
	int code);

  inline void
  unlock_execution ()
  {
    execution_mutex_.unlock ();
  }

  inline bd_t
  inita () const
  {
    return init_buffer_a_;
  }

  inline bd_t
  initb () const
  {
    return init_buffer_b_;
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

  struct mapped_area_overlaps {
    physical_address_t physical_begin;
    physical_address_t physical_end;

    mapped_area_overlaps (const physical_address_t& begin,
			  const physical_address_t& end) :
      physical_begin (begin),
      physical_end (end)
    { }

    inline bool
    operator () (const mapped_area* const x) const
    {
      return
	(x->physical_begin <= physical_begin && physical_begin < x->physical_end) ||
	(physical_begin <= x->physical_begin && x->physical_begin < physical_end);
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
    for (logical_address_t address = stack_area_->begin (); address != stack_area_->end (); address += PAGE_SIZE) {
      vm::map (address, vm::zero_frame (), vm::USER, vm::MAP_COPY_ON_WRITE);
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
  
  inline void
  remove_vm_area (vm_area_base* area)
  {
    // TODO:  Use binary search.
    memory_map_type::iterator pos = find (memory_map_.begin (), memory_map_.end (), area);
    kassert (pos != memory_map_.end ());
    memory_map_.erase (pos);
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

  inline pair<void*, lily_error_t>
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
	  
	  return make_pair (reinterpret_cast<void*> (old_end), LILY_ERROR_SUCCESS);
	}
	else {
	  // Failure.
	  return make_pair ((void*)0, LILY_ERROR_NOMEM);
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

	return make_pair (reinterpret_cast<void*> (old_end), LILY_ERROR_SUCCESS);
      }
    }
    else {
      return make_pair (reinterpret_cast<void*> (heap_area_->end ()), LILY_ERROR_SUCCESS);
    }
  }

  // TODO:  Limit the number and size of buffers.

  inline pair<bd_t, lily_error_t>
  buffer_create (size_t size)
  {
    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* b = new buffer (size);
    bd_to_buffer_map_.insert (make_pair (bd, b));

    return make_pair (bd, LILY_ERROR_SUCCESS);
  }

  inline pair<bd_t, lily_error_t>
  buffer_copy (bd_t other)
  {
    
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (other);
    if (bpos == bd_to_buffer_map_.end ()) {
      // Buffer does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }

    shared_ptr<buffer> b = bpos->second;

    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    buffer* n = new buffer (*b, 0, b->size ());
    bd_to_buffer_map_.insert (make_pair (bd, n));
    
    return make_pair (bd, LILY_ERROR_SUCCESS);
  }
  
  inline bd_t
  buffer_create (const shared_ptr<buffer>& other)
  {
    // Generate an id.
    bd_t bd = generate_bd ();
    
    // Create the buffer and insert it into the map.
    shared_ptr<buffer> b = shared_ptr<buffer> (new buffer (*other));
    bd_to_buffer_map_.insert (make_pair (bd, b));
    
    return bd;
  }

  inline pair<int, lily_error_t>
  buffer_resize (bd_t bd,
		 size_t size)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos == bd_to_buffer_map_.end ()) {
      // Buffer does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }

    shared_ptr<buffer> b = bpos->second;
    if (b->begin () != 0) {
      // Buffer was mapped.
      return make_pair (-1, LILY_ERROR_INVAL);
    }

    b->resize (size);
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  inline pair<size_t, lily_error_t>
  buffer_append (bd_t dst,
		 bd_t src)
  {
    bd_to_buffer_map_type::const_iterator dst_pos = bd_to_buffer_map_.find (dst);
    bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
    if (dst_pos == bd_to_buffer_map_.end () ||
	src_pos == bd_to_buffer_map_.end ()) {
      // One of the buffers does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }

    shared_ptr<buffer> d = dst_pos->second;
    shared_ptr<buffer> s = src_pos->second;
    
    if (d->begin () != 0) {
      // The destination is mapped.
      return make_pair (-1, LILY_ERROR_INVAL);
    }

    // Append.
    d->append (*s, 0, s->size ());
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  inline pair<int, lily_error_t>
  buffer_assign (bd_t dest,
		 bd_t src)
  {
    bd_to_buffer_map_type::const_iterator dest_pos = bd_to_buffer_map_.find (dest);
    bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
    if (dest_pos == bd_to_buffer_map_.end () ||
	src_pos == bd_to_buffer_map_.end ()) {
      // One of the buffers does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }

    shared_ptr<buffer> dest_b = dest_pos->second;
    shared_ptr<buffer> src_b = src_pos->second;

    // Truncate and append.
    dest_b->resize (0);
    dest_b->append (*src_b, 0, src_b->size ());

    // Append.
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

private:
  inline void*
  buffer_map (const shared_ptr<buffer>& b)
  {
    kassert (heap_area_ != 0);
    kassert (stack_area_ != 0);
    
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
	memory_map_.insert (prev.base (), b.get ());
	// Success.
	return (void*)b->begin ();
      }
    }
    
    // Couldn't find a big enough hole.
    return 0;
  }

public:

  inline pair<void*, lily_error_t>
  buffer_map (bd_t bd)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos == bd_to_buffer_map_.end ()) {
      // The buffer does not exist.
      return make_pair ((void*)0, LILY_ERROR_BDDNE);
    }

    shared_ptr<buffer> b = bpos->second;
    
    if (b->size () == 0) {
      // The buffer is empty.
      return make_pair ((void*)0, LILY_ERROR_INVAL);
    }

    if (b->begin () != 0) {
      // The buffer is already mapped.  Return the address.
      return make_pair (reinterpret_cast<void*> (b->begin ()), LILY_ERROR_SUCCESS);
    }

    // Map the buffer.
    void* ptr = buffer_map (b);

    if (ptr != 0) {
      return make_pair (ptr, LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair ((void*)0, LILY_ERROR_NOMEM);
    }
  }

private:
  inline void
  buffer_unmap (const shared_ptr<buffer>& b)
  {
    if (b->begin () != 0) {
      // Remove from the memory map.
      remove_vm_area (b.get ());
      
      // Unmap the buffer.	
      b->unmap ();
    }
  }

public:
  inline pair<int, lily_error_t>
  buffer_unmap (bd_t bd)
  {
    bd_to_buffer_map_type::iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos == bd_to_buffer_map_.end ()) {
      // The buffer does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }
    
    shared_ptr<buffer> b = bpos->second;
    buffer_unmap (b);

    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  inline pair<int, lily_error_t>
  buffer_destroy (bd_t bd)
  {
    bd_to_buffer_map_type::iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos != bd_to_buffer_map_.end ()) {
      shared_ptr<buffer> b = bpos->second;

      if (b->begin () != 0) {
	// Remove from the memory map.
	remove_vm_area (b.get ());
	// Buffer will be unmapped automatically when destroyed.
      }

      // Remove from the bd map.
      bd_to_buffer_map_.erase (bpos);

      return make_pair (0, LILY_ERROR_SUCCESS);
    }
    else {
      // The buffer does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }
  }

  inline pair<size_t, lily_error_t>
  buffer_size (bd_t bd)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos != bd_to_buffer_map_.end ()) {
      return make_pair (bpos->second->size (), LILY_ERROR_SUCCESS);
    }
    else {
      // The buffer does not exist.
      return make_pair (-1, LILY_ERROR_BDDNE);
    }
  }

  inline shared_ptr<buffer>
  lookup_buffer (bd_t bd)
  {
    bd_to_buffer_map_type::const_iterator bpos = bd_to_buffer_map_.find (bd);
    if (bpos != bd_to_buffer_map_.end ()) {
      return bpos->second;
    }
    else {
      return shared_ptr<buffer> ();
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

  pair<bid_t, lily_error_t>
  bind (const shared_ptr<automaton>& ths,
	aid_t output_aid,
	ano_t output_ano,
	int output_parameter,
	aid_t input_aid,
	ano_t input_ano,
	int input_parameter);

  inline pair<int, lily_error_t>
  unbind (const shared_ptr<automaton>& ths,
	  bid_t bid)
  {
    kassert (ths.get () == this);
    
    // Look up the binding.
    bid_to_binding_map_type::iterator pos = bid_to_binding_map_.find (bid);
    if (pos == bid_to_binding_map_.end ()) {
      return make_pair (-1, LILY_ERROR_BIDDNE);
    }
    
    shared_ptr<binding> binding = pos->second;
    
    // Are we the owner?
    if (binding->owner != ths) {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
    
    unbind (binding, true, true, true);
    
    return make_pair (0, LILY_ERROR_SUCCESS);
  }
  
  /*
   * I/O
   */

public:
  inline pair<int, lily_error_t>
  getmonotime (mono_time_t* t)
  {
    // Check the name.
    if (!verify_span (t, sizeof (mono_time_t))) {
      return make_pair (-1, LILY_ERROR_INVAL);
    }

    irq_handler::getmonotime (t);
    
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  // The automaton is requesting that the physical memory from [source, source + size) appear at [destination, destination + size) in its address space.
  // The destination and source are aligned to a page boundary and the size is rounded up to a multiple of the page size.
  // Map can fail for the following reasons:
  // *  The automaton is not privileged.
  // *  The low order bits of the destination and source do not agree when interpretted using the page size.
  // *  Size is zero.
  // *  Part of the source lies outside the regions of memory marked for memory-mapped I/O.
  // *  Part of the source is already claimed by some other automaton.  (Mapping the same region twice is an error.)
  // *  The destination address is not available.
  inline pair<int, lily_error_t>
  map (const void* destination,
       const void* source,
       size_t size)
  {
    logical_address_t const destination_begin = align_down (reinterpret_cast<logical_address_t> (destination), PAGE_SIZE);
    logical_address_t const destination_end = align_up (reinterpret_cast<logical_address_t> (destination) + size, PAGE_SIZE);
    physical_address_t const source_begin = align_down (reinterpret_cast<physical_address_t> (source), PAGE_SIZE);
    physical_address_t const source_end = align_up (reinterpret_cast<physical_address_t> (source) + size, PAGE_SIZE);

    if (!privileged_) {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }

    if ((reinterpret_cast<logical_address_t> (destination) & (PAGE_SIZE - 1)) != (reinterpret_cast<physical_address_t> (source) & (PAGE_SIZE - 1))) {
      return make_pair (-1, LILY_ERROR_INVAL);
    }

    if (size == 0) {
      return make_pair (-1, LILY_ERROR_INVAL);
    }

    if (find_if (all_mapped_areas_.begin (), all_mapped_areas_.end (), mapped_area_overlaps (destination_begin, destination_end)) != all_mapped_areas_.end ()) {
      return make_pair (-1, LILY_ERROR_ALREADY);
    }

    if (!vm_area_is_free (destination_begin, destination_end)) {
      return make_pair (-1, LILY_ERROR_ALREADY);
    }
    
    mapped_area* area = new mapped_area (destination_begin,
  					 destination_end,
  					 source_begin,
  					 source_end);

    insert_vm_area (area);
    mapped_areas_.push_back (area);
    all_mapped_areas_.push_back (area);

    physical_address_t pa = source_begin;
    for (logical_address_t la = destination_begin; la != destination_end; la += PAGE_SIZE, pa += PAGE_SIZE) {
      vm::map (la, physical_address_to_frame (pa), vm::USER, vm::MAP_READ_WRITE, false);
    }

    // Success.
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  inline pair<int, lily_error_t>
  unmap (const void* destination)
  {
    mapped_areas_type::iterator pos = find_if (mapped_areas_.begin (), mapped_areas_.end (), contains_address (destination));
    if (pos == mapped_areas_.end ()) {
      return make_pair (-1, LILY_ERROR_NOT);
    }

    mapped_area* area = *pos;

    physical_address_t pa = area->physical_begin;
    for (logical_address_t la = area->begin ();
	 la != area->end ();
	 la += PAGE_SIZE, pa += PAGE_SIZE) {
      vm::unmap (la);
    }

    mapped_areas_type::iterator pos2 = find (all_mapped_areas_.begin (), mapped_areas_.end (), area);
    kassert (pos2 != all_mapped_areas_.end ());
    all_mapped_areas_.erase (pos2);
    mapped_areas_.erase (pos);
    remove_vm_area (area);
    
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  // The automaton is requesting access to the specified I/O port.
  inline pair<int, lily_error_t>
  reserve_port (unsigned short port)
  {
    if (!privileged_) {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }

    if (reserved_ports_[port]) {
      // Port is already reserved.
      return make_pair (-1, LILY_ERROR_ALREADY);
    }

    // Reserve the port.
    reserved_ports_[port] = true;
    port_set_.insert (port);

    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  inline pair<int, lily_error_t>
  unreserve_port (unsigned short port)
  {
    port_set_type::iterator pos = port_set_.find (port);
    if (pos == port_set_.end ()) {
      return make_pair (-1, LILY_ERROR_NOT);
    }

    reserved_ports_[port] = false;
    port_set_.erase (pos);

    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  inline pair<uint8_t, lily_error_t>
  inb (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inb (port), LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
  }

  inline pair<int, lily_error_t>
  outb (uint16_t port,
	uint8_t value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outb (port, value);
      return make_pair (0, LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
  }

  inline pair<uint16_t, lily_error_t>
  inw (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inw (port), LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
  }

  inline pair<int, lily_error_t>
  outw (uint16_t port,
	uint16_t value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outw (port, value);
      return make_pair (0, LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
  }

  inline pair<uint32_t, lily_error_t>
  inl (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inl (port), LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
  }

  inline pair<int, lily_error_t>
  outl (uint16_t port,
	uint32_t value)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      io::outl (port, value);
      return make_pair (0, LILY_ERROR_SUCCESS);
    }
    else {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
  }

  inline pair<int, lily_error_t>
  subscribe_irq (const shared_ptr<automaton>& ths,
		 int irq,
		 ano_t action_number,
		 int parameter)
  {
    kassert (ths.get () == this);
    
    if (!privileged_) {
      return make_pair (-1, LILY_ERROR_PERMISSION);
    }
    
    if (irq == irq_handler::PIT_IRQ ||
	irq == irq_handler::CASCADE_IRQ ||
	irq < irq_handler::IRQ_BASE ||
	irq >= irq_handler::IRQ_LIMIT) {
      return make_pair (-1, LILY_ERROR_INVAL);
    }
    
    // Find the action.
    const paction* action = find_action (action_number);
    if (action == 0 || action->type != SYSTEM_INPUT) {
      return make_pair (-1, LILY_ERROR_ANODNE);
    }
    
    // Correct the parameter.
    if (action->parameter_mode == NO_PARAMETER) {
      parameter = 0;
    }
    
    if (irq_map_.find (irq) != irq_map_.end ()) {
      /* Already subscribed.  Note that we don't check the action. */
      return make_pair (-1, LILY_ERROR_ALREADY);
    }
    
    caction c (ths, action, parameter);
    
    irq_map_.insert (make_pair (irq, c));
    irq_handler::subscribe (irq, c);
    
    return make_pair (0, LILY_ERROR_SUCCESS);
  }
  
  inline pair<int, lily_error_t>
  unsubscribe_irq (int irq)
  {
    irq_map_type::iterator pos = irq_map_.find (irq);
    if (pos == irq_map_.end ()) {
      return make_pair (-1, LILY_ERROR_NOT);
    }
    
    irq_handler::unsubscribe (pos->first, pos->second);
    irq_map_.erase (pos);
    
    return make_pair (0, LILY_ERROR_SUCCESS);
  }

  /*
   * SUBSCRIPTIONS
   */

public:
  pair<int, lily_error_t>
  log (const char* message,
       size_t message_size);

  /*
   * CREATION AND DESTRUCTION
   */

  automaton () :
    aid_ (-1),
    regenerate_description_ (false),
    parent_ (0),
    init_buffer_a_ (-1),
    init_buffer_b_ (-1),
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
    /*
      Address the instance variables:
      
      aid_
      name_
      ano_to_action_map_
      name_to_action_map_
      description_
      regenerate_description_
      parent_
      children_
      execution_mutex_
      init_buffer_a_
      init_buffer_b_
      page_directory_
      memory_map_
      heap_area_
      stack_area_
      current_bd_
      bd_to_buffer_map_
      bound_outputs_map_
      bound_inputs_map_
      owned_bindings_
      privileged_
      mapped_areas_
      port_set_
      irq_map_
      automaton_subscriptions_
      subscribers_;
      binding_subscriptions_
    */

    kassert (aid_ == -1);
    
    // Nothing for name_.
    for (ano_to_action_map_type::const_iterator pos = ano_to_action_map_.begin ();
    	 pos != ano_to_action_map_.end ();
    	 ++pos) {
      delete pos->second;
    }
    // Nothing for name_to_action_map_.
    // Nothing for description_.
    // Nothing for regenerate_description_.
    kassert (parent_.get () == 0);
    kassert (children_.empty ());
    kassert (!execution_mutex_.locked ());
    // Nothing for init_buffer_a_.
    // Nothing for init_buffer_b_.

    // We need to switch to this automaton's page directory to take apart the memory map.
    // Either we are already using this automaton's memory map or we are using someone else's (including the kernel).
    // If we were using our own, pretend we were using the kernel.
    physical_address_t old_page_directory;
    if (vm::get_directory () == page_directory) {
      old_page_directory = vm::get_kernel_page_directory_physical_address ();
    }
    else {
      old_page_directory = vm::switch_to_directory (page_directory);
    }

    // Nothing for current_bd_.
    for (bd_to_buffer_map_type::const_iterator pos = bd_to_buffer_map_.begin ();
	 pos != bd_to_buffer_map_.end ();
	 ++pos) {
      // Remove from the memory map.
      if (pos->second->begin () != 0) {
	remove_vm_area (pos->second.get ());
      }
    }
    // This removes references to the buffers.
    bd_to_buffer_map_.clear ();

    // Nothing for heap_area_.
    // Nothing for stack_area_.

    /*
      The tricky part is returning all of the frames used by this automaton.
      Let's start by reviewing the memory map of an automaton.
      
      0-KERNEL_VIRTUAL_BASE		: Memory mapped I/O regions, text, data, heap, buffers, stack in roughly that order
      KERNEL_VIRTUAL_BASE - PAGING_AREA : Kernel text and data
      PAGING_AREA - end			: Page tables and page directory.

      The frames occupied by memory mapped I/O regions are not managed by the frame manager, thus, we should ignore them.
      Note that all of the mapped_areas were removed from memory_map_ in destroy ().

      Buffers need to be synchronized before they are destroyed due to copy-on-write semantics.
      The preceding code destroys all of the buffers.

      Thus, we need to be concerned with (1) the text/data/heap/stack frames, (2) the kernel, and (3) the paging area.
      First, we will remove (1).
     */

    for (memory_map_type::const_iterator pos = memory_map_.begin ();
	 pos != memory_map_.end ();
	 ++pos) {
      for (logical_address_t la = (*pos)->begin ();
	   la < (*pos)->end ();
	   la += PAGE_SIZE) {
	// No error if not mapped.
	// We need this because regions overlap.
	vm::unmap (la, true, false);
      }

      delete (*pos);
    }

    /* To remove (2) and (3) we scan the page directory for page tables that are present and decref the frame.
       This works because all of these frames including the kernel are reference counted.
     */
    vm::page_directory* dir = vm::get_page_directory ();
    for (page_table_idx_t idx = 0; idx != PAGE_ENTRY_COUNT; ++idx) {
      if (dir->entry[idx].present_ == vm::PRESENT) {
	frame_manager::decref (dir->entry[idx].frame_);
      }
    }

    /* Switch back to the old page directory. */
    vm::switch_to_directory (old_page_directory);

    /* The page directory has a reference from when we allocated it.
       Drop the reference count. */
    size_t count = frame_manager::decref (physical_address_to_frame (page_directory));
    kassert (count == 0);
    
    kassert (bound_outputs_map_.empty ());
    kassert (bound_inputs_map_.empty ());
    kassert (owned_bindings_.empty ());
    // Nothing for privileged_.
    kassert (mapped_areas_.empty ());
    kassert (port_set_.empty ());
    kassert (irq_map_.empty ());
  }

};

#endif /* __automaton_hpp__ */
