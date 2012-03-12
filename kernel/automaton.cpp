#include "automaton.hpp"
#include "scheduler.hpp"

aid_t automaton::current_aid_ = 0;
automaton::aid_to_automaton_map_type automaton::aid_to_automaton_map_;
automaton::registry_map_type automaton::registry_map_;

bitset<ONE_MEGABYTE / PAGE_SIZE> automaton::mmapped_frames_;
bitset<65536> automaton::reserved_ports_;

automaton::binding_set_type automaton::empty_set_;

void
automaton::finish (ano_t action_number,
		   int parameter,
		   bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  if (action_number > LILY_ACTION_NO_ACTION) {
    const paction* action = find_action (action_number);
    if (action != 0) {
      /* Correct the parameter. */
      switch (action->parameter_mode) {
      case NO_PARAMETER:
	parameter = 0;
	break;
      case PARAMETER:
      case AUTO_PARAMETER:
	/* No correction necessary. */
	break;
      }
      
      switch (action->type) {
      case OUTPUT:
      case INTERNAL:
	scheduler::schedule (caction (action, parameter));
	break;
      case INPUT:
      case SYSTEM_INPUT:
	// BUG:  Can't schedule non-local actions.
	kassert (0);
	break;
      }
    }
    else {
      // BUG:  Scheduled an action that doesn't exist.
      kassert (0);
    }
  }
  
  scheduler::finish (output_fired, bda, bdb);
}

pair<bid_t, int>
automaton::bind (aid_t output_aid,
		 ano_t output_ano,
		 int output_parameter,
		 aid_t input_aid,
		 ano_t input_ano,
		 int input_parameter)
{
  automaton* output_automaton = automaton::lookup (output_aid);
  if (output_automaton == 0) {
    // Output automaton DNE.
    return make_pair (-1, LILY_SYSCALL_EOAIDDNE);
  }
  
  automaton* input_automaton = automaton::lookup (input_aid);
  if (input_automaton == 0) {
    // Input automaton DNE.
    return make_pair (-1, LILY_SYSCALL_EIAIDDNE);
  }
  
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
  caction oa (output_action, output_parameter);
  caction ia (input_action, input_parameter);
  
  // Check that the input action is not bound.
  // (Also checks that the binding does not exist.)
  if (input_automaton->is_input_bound (ia)) {
    return make_pair (-1, LILY_SYSCALL_EINVAL);
  }
  
  // Check that the output action is not bound to an action in the input automaton.
  if (output_automaton->is_output_bound_to_automaton (oa, input_automaton)) {
    return make_pair (-1, LILY_SYSCALL_EINVAL);
  }
  
  // Bind.
  binding* b = new binding (oa, ia, this);
  output_automaton->bind_output (b);
  input_automaton->bind_input (b);
  this->bind (b);
  
  // TODO:  Remove this.
  // Schedule the output action.
  scheduler::schedule (caction (output_action, output_parameter));
  
  return make_pair (b->bid (), LILY_SYSCALL_ESUCCESS);
}

pair<aid_t, int>
automaton::create (bd_t text_bd,
		   size_t /*text_size*/,
		   bd_t bda,
		   bd_t bdb,
		   const char* name,
		   size_t name_size,
		   bool retain_privilege)
{
  // Check the name.
  if (name_size != 0 && (!verify_span (name, name_size) || name[name_size - 1] != 0)) {
    return make_pair (-1, LILY_SYSCALL_EINVAL);
  }
  
  kstring name_str (name, name_size);
  if (name_size != 0 && registry_map_.find (name_str) != registry_map_.end ()) {
    return make_pair (-1, LILY_SYSCALL_EEXISTS);
  }

  // Find the text buffer.
  buffer* text_buffer = lookup_buffer (text_bd);
  
  if (text_buffer == 0) {
    // Buffer does not exist.
    return make_pair (-1, LILY_SYSCALL_EBDDNE);
  }
  
  // Synchronize the buffer so the frames listed in the buffer are correct.
  text_buffer->sync (0, text_buffer->size ());
  
  // If the buffer is not mapped, there are no problems.
  // If the buffer is mapped, we have three options:
  // 1.  Copy the buffer.
  // 2.  Unmap the buffer and then map it back at it original location.
  // 3.  Save the location of the buffer and temporatily override it.
  // Option 3 has the least overhead so that's what we'll do.
  
  logical_address_t const begin = text_buffer->begin ();
  logical_address_t const end = text_buffer->end ();
  
  text_buffer->override (0, 0);
  
  // Switch to the kernel page directory.  So we can map the text of the automaton.
  physical_address_t old = vm::switch_to_directory (vm::get_kernel_page_directory_physical_address ());
  
  // Create the automaton.
  automaton* child = create_automaton (this, text_buffer, text_buffer->size () * PAGE_SIZE, name_str, retain_privilege && privileged_);
  
  // Switch back.
  vm::switch_to_directory (old);
  
  text_buffer->override (begin, end);
  
  // Find and synchronize the data buffers so the frames listed in the buffers are correct.
  buffer* buffer_a = lookup_buffer (bda);
  if (buffer_a != 0) {
    buffer_a->sync (0, buffer_a->size ());
  }
  buffer* buffer_b = lookup_buffer (bdb);
  if (buffer_b != 0) {
    buffer_b->sync (0, buffer_b->size ());
  }
  
  // Schedule the init action.
  const paction* action = child->find_action (LILY_ACTION_INIT);
  if (action != 0) {
    // Replace the buffers with copies.
    if (buffer_a != 0) {
      buffer_a = new buffer (*buffer_a);
    }
    if (buffer_b != 0) {
      buffer_b = new buffer (*buffer_b);
    }
    
    scheduler::schedule (caction (action, child->aid (), buffer_a, buffer_b));
  }
  
  return make_pair (child->aid (), LILY_SYSCALL_ESUCCESS);
}

// TODO:  Make this reentrant.
// A list of frames used when creating automata.
typedef unordered_map<logical_address_t, pair<frame_t, vm::map_mode_t> > frame_map_type;
static frame_map_type frame_map_;

typedef vector<pair<logical_address_t, logical_address_t> > clear_list_type;
static clear_list_type clear_list_;

automaton*
automaton::create_automaton (automaton* parent,
			     buffer* text,
			     size_t text_size,
			     const kstring& name,
			     bool privileged)
{
  // Map the text of the initial automaton just after low memory.
  const logical_address_t text_begin = INITIAL_LOGICAL_LIMIT - KERNEL_VIRTUAL_BASE;
  const logical_address_t text_end = text_begin + text_size;
  text->map_begin (text_begin);
  
  // Parse the file.
  elf::parser hp;
  if (!hp.parse (text_begin, text_end)) {
    // BUG:  Couldn't parse the program.
    kassert (0);
  }
  
  // BUG:  Check the memory map from the parse.
  // BUG:  Check the set of actions from the parse.
  
  // Create the automaton.
  
  // Create the automaton and insert it into the map.
  automaton* a = new automaton (parent, name, privileged);
  if (parent != 0) {
    parent->add_child (a);
    // Remove the reference count from creation.
    a->decref ();
  }
  
  // Add to the scheduler.
  scheduler::add_automaton (a);
  
  // Add the actions to the automaton.
  for (elf::parser::action_list_type::iterator pos = hp.action_list.begin (); pos != hp.action_list.end (); ++pos) {
    switch (pos->action_number) {
    case LILY_ACTION_NO_ACTION:
      // BUG:  The action number LILY_ACTION_NO_ACTION is reserved.
      kassert (0);
      break;
    case LILY_ACTION_INIT:
      if (pos->type != LILY_ACTION_SYSTEM_INPUT || pos->parameter_mode != PARAMETER) {
	// BUG:  These must have the correct type.
	kassert (0);
      }
      // Fall through.
    default:
      if (!a->add_action (new paction (a, pos->type, pos->parameter_mode, pos->action_entry_point, pos->action_number))) {
	// BUG:  The action conflicts with an existing action.  Be sure to delete the rest of the pointers.
	kassert (0);
      }
      break;
    }
  }
  
  // Form a description of the actions.
  kstring description;
  {
    size_t action_count = hp.action_list.size ();
    description.append (&action_count, sizeof (action_count));
  }
  
  for (elf::parser::action_list_type::iterator pos = hp.action_list.begin (); pos != hp.action_list.end (); ++pos) {
    description.append (&pos->type, sizeof (pos->type));
    description.append (&pos->parameter_mode, sizeof (pos->parameter_mode));
    description.append (&pos->action_number, sizeof (pos->action_number));
    size_t size = pos->action_name.size ();
    description.append (&size, sizeof (size));
    size = pos->action_description.size ();
    description.append (&size, sizeof (size));
    description.append (pos->action_name.c_str (), pos->action_name.size ());
    description.append (pos->action_description.c_str (), pos->action_description.size ());
  }
  
  a->set_description (description);
  
  // Create the memory map for the automaton.
  frame_map_.clear ();
  clear_list_.clear ();
  for (elf::parser::program_header_iterator e = hp.program_header_begin (); e != hp.program_header_end (); ++e) {
    vm::map_mode_t map_mode = ((e->permissions & elf::WRITE) != 0) ? vm::MAP_COPY_ON_WRITE : vm::MAP_READ_ONLY;
    
    size_t s;
    // Initialized data.
    for (s = 0; s < e->file_size; s += PAGE_SIZE) {
      pair<frame_map_type::iterator, bool> r = frame_map_.insert (make_pair (align_down (e->virtual_address + s, PAGE_SIZE), make_pair (vm::logical_address_to_frame (text_begin + e->offset + s), map_mode)));
      if (!r.second && map_mode == vm::MAP_COPY_ON_WRITE) {
	// Already existed and needs to be copy-on-write.
	r.first->second.second = vm::MAP_COPY_ON_WRITE;
      }
    }
    
    // Clear the tiny region between the end of initialized data and the first unitialized page.
    if (e->file_size < e->memory_size) {
      logical_address_t begin = e->virtual_address + e->file_size;
      logical_address_t end = e->virtual_address + e->memory_size;
      clear_list_.push_back (make_pair (begin, end));
    }
    
    // Uninitialized data.
    for (; s < e->memory_size; s += PAGE_SIZE) {
      pair<frame_map_type::iterator, bool> r = frame_map_.insert (make_pair (align_down (e->virtual_address + s, PAGE_SIZE), make_pair (vm::zero_frame (), map_mode)));
      if (!r.second && map_mode == vm::MAP_COPY_ON_WRITE) {
	// Already existed and needs to be copy-on-write.
	r.first->second.second = vm::MAP_COPY_ON_WRITE;
      }
    }
    
    if (!a->vm_area_is_free (e->virtual_address, e->virtual_address + e->memory_size)) {
      // BUG:  The area conflicts with the existing memory map.
      kassert (0);
    }
    
    a->insert_vm_area (new vm_area_base (e->virtual_address, e->virtual_address + e->memory_size));
  }
  
  if (!a->insert_heap_and_stack ()) {
    // BUG
    kassert (0);
  }

  // Switch to the automaton.
  physical_address_t old = vm::switch_to_directory (a->page_directory_);
  
  // We can only use data in the kernel, i.e., we can't use automaton_text or hp.
  
  // Map all the frames.
  for (frame_map_type::const_iterator pos = frame_map_.begin ();
       pos != frame_map_.end ();
       ++pos) {
    vm::map (pos->first, pos->second.first, vm::USER, pos->second.second);
  }
  
  // Clear.
  for (clear_list_type::const_iterator pos = clear_list_.begin ();
       pos != clear_list_.end ();
       ++pos) {
    memset (reinterpret_cast<void*> (pos->first), 0, pos->second - pos->first);
  }
  
  // Map the heap and stack while using the automaton's page directory.
  a->map_heap_and_stack ();
  
  // Switch back.
  vm::switch_to_directory (old);
  
  // Unmap the text.
  text->unmap ();
  
  return a;
}

  // void
  // remove_from_scheduler (automaton* a)
  // {
  //   scheduler::remove_automaton (a);

  //   for (automaton::children_set_type::const_iterator pos = a->children_begin ();
  //   	 pos != a->children_end ();
  //   	 ++pos) {
  //     remove_from_scheduler (*pos);
  //   }
  // }

void
automaton::execute (const caction& action_,
		    buffer* output_buffer_a_,
		    buffer* output_buffer_b_)
{
  kassert (action_.action != 0);
  kassert (action_.action->automaton == this);
  
  if (enabled_) {
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
    vm::switch_to_directory (page_directory_);
    
    uint32_t* stack_pointer = reinterpret_cast<uint32_t*> (stack_area_->end ());
    
    // These instructions serve a dual purpose.
    // First, they set up the cdecl calling convention for actions.
    // Second, they force the stack to be created if it is not.
    
    switch (action_.action->type) {
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
    *--stack_pointer = action_.parameter;
    
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
    *--stack_pointer = reinterpret_cast<uint32_t> (action_.action->action_entry_point);
    
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
  else {
    scheduler::finish (false, -1, -1);
  }
}
