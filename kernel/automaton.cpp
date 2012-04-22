#include "automaton.hpp"
#include "scheduler.hpp"
#include "elf.hpp"

aid_t automaton::current_aid_ = 0;
automaton::aid_to_automaton_map_type automaton::aid_to_automaton_map_;

bid_t automaton::current_bid_ = 0;
automaton::bid_to_binding_map_type automaton::bid_to_binding_map_;

automaton::mapped_areas_type automaton::all_mapped_areas_;
bitset<65536> automaton::reserved_ports_;

automaton::log_event_map_type automaton::log_event_map_;
automaton::system_event_map_type automaton::system_event_map_;

pair<int, lily_error_t>
automaton::log (const char* message,
		size_t message_size)
{
  if (!verify_span (message, message_size)) {
    return make_pair (-1, LILY_ERROR_INVAL);
  }

  const size_t total_size = sizeof (log_event_t) + message_size;
  size_t page_count = align_up (total_size, PAGE_SIZE) / PAGE_SIZE;

  // Create a buffer for the description.
  shared_ptr<buffer> b (new buffer (page_count));
  log_event_t* le = static_cast <log_event_t*> (buffer_map (b));
  le->aid = aid_;
  irq_handler::getmonotime (&le->time);
  le->message_size = message_size;
  memcpy (&le->message[0], message, message_size);

  console::fmtflags flags = kout.flags ();
  bool print_header = true;
  for (size_t idx = 0; idx != message_size; ++idx) {
    if (print_header) {
      print_header = false;
      kout << "[" << setfill (' ') << setw (10) << left << le->time.seconds << "." << setfill ('0') << setw (3) << le->time.nanoseconds / 1000000 << "] " << le->aid << " ";
    }
    kout.put (message[idx]);
    if (message[idx] == '\n') {
      print_header = true;
    }
  }
  if (!print_header) {
    kout << endl;
  }
  kout.flags (flags);
  
  buffer_unmap (b);
  
  for (log_event_map_type::const_iterator pos = log_event_map_.begin ();
       pos != log_event_map_.end ();
       ++pos) {
    scheduler::schedule (caction (pos->first, pos->second, 0, b, shared_ptr<buffer> ()));
  }

  return make_pair (0, LILY_ERROR_SUCCESS);
}

pair<int, lily_error_t>
automaton::schedule (const shared_ptr<automaton>& ths,
		     ano_t action_number,
		     int parameter)
{
  kassert (ths.get () == this);

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
      scheduler::schedule (caction (ths, action, parameter));
      return make_pair (0, LILY_ERROR_SUCCESS);
      break;
    case INPUT:
    case SYSTEM_INPUT:
      return make_pair (-1, LILY_ERROR_INVAL);
      break;
    }
  }
  else {
    return make_pair (-1, LILY_ERROR_ANODNE);
  }
}

// The automaton would like to no longer exist.
void
automaton::exit (const shared_ptr<automaton>& ths,
		 int code)
{
  kassert (ths.get () == this);
      
  shared_ptr<automaton> parent = parent_;
  
  unsubscribe (ths);
  destroy (ths);
  
  if (parent.get () != 0) {
    size_t count = parent->children_.erase (ths);
    kassert (count == 1);
  }
  
  scheduler::finish (false, -1, -1);
}

/*
  This function is called in two contexts:
  (1)  In kmain to create the initial automaton.
  (2)  In create to create automata.  (create is a wrapper around this function that checks some arguments)

  This function uses an optimistic strategy where we first create an empty automaton and then flesh it out as we parse the text.
  The dynamic resources are associated with a scoped object so that all dynamic resources are freed when an error is encountered.
  This is similiar to an auto pointer in the STL or scroped locking.

  The basic procedure implemented by the function is to map the text of the automaton, parse it, unmap it, and then install it if successful.
  Installing is the process where global data structures are updated.
  Context (2) acquires the id_lock_ before invoking this function for thread safety.
*/

pair<shared_ptr<automaton>, lily_error_t>
automaton::create_automaton (const shared_ptr<automaton>& parent,
			     bool privileged,
			     const shared_ptr<buffer>& text,
			     size_t text_size,
			     const shared_ptr<buffer>& buffer_a_,
			     const shared_ptr<buffer>& buffer_b_)
{
  // If the parent exists, we enter with the id_mutex_ locked and the parent's mod_mutex_ locked.

  // Create the automaton.
  shared_ptr<automaton> child = shared_ptr<automaton> (new automaton ());

  // First, we want to map the text of the automaton in the kernel's address space.

  // Synchronize the buffer so the frames listed in the buffer are correct.
  // We must do this before switching to the kernel.
  text->sync (0, text->size ());

  // If the text buffer is not mapped in the current page directory, there are no problems.
  // If the text buffer is mapped, we have three options:
  // 1.  Copy the buffer.
  // 2.  Unmap the buffer and then map it back at its original location.
  // 3.  Save the location of the buffer and temporarily override it.
  // Option 3 has the least overhead so that's what we'll do.
  logical_address_t const begin = text->begin ();
  logical_address_t const end = text->end ();
  text->override (0, 0);

  // Switch to the kernel page directory.
  physical_address_t original_directory = vm::switch_to_directory (vm::get_kernel_page_directory_physical_address ());

  // Map the text of the initial automaton just after low memory.
  const logical_address_t text_begin = INITIAL_LOGICAL_LIMIT - KERNEL_VIRTUAL_BASE;
  const logical_address_t text_end = text_begin + text_size;
  text->map_begin (text_begin);

  // Parse the file.
  int parse_result = elf::parse (child, text_begin, text_end);

  if (parse_result == 0) {
    // Generate an id and insert into the aid to automaton map.
    aid_t child_aid = current_aid_;
    while (aid_to_automaton_map_.find (child_aid) != aid_to_automaton_map_.end ()) {
      child_aid = max (child_aid + 1, 0); // Handles overflow.
    }
    current_aid_ = max (child_aid + 1, 0);
    
    aid_to_automaton_map_.insert (make_pair (child_aid, child));
    
    // Add the child to the parent.
    if (parent.get () != 0) {
      parent->children_.insert (child);
    }

    child->aid_ = child_aid;
    child->parent_ = parent;
    child->privileged_ = privileged;
    
    // Add to the scheduler.
    scheduler::add_automaton (child);
    
    if (buffer_a_.get () != 0) {
      child->init_buffer_a_ = child->buffer_create (buffer_a_);
    }
    if (buffer_b_.get () != 0) {
      child->init_buffer_b_ = child->buffer_create (buffer_b_);
    }
    
    // Schedule the init action and look for the system_event action.
    const kstring init_name ("init");
    const kstring log_event_name ("log_event");
    const kstring system_event_name ("system_event");
    for (ano_to_action_map_type::const_iterator pos = child->ano_to_action_map_.begin ();
	 pos != child->ano_to_action_map_.end ();
	 ++pos) {
      if (pos->second->name == init_name && pos->second->type == INTERNAL) {
	scheduler::schedule (caction (child, pos->second, child_aid));
      }
      else if (pos->second->name == log_event_name && pos->second->type == SYSTEM_INPUT) {
	log_event_map_.insert (make_pair (child, pos->second));
      }
      else if (pos->second->name == system_event_name && pos->second->type == SYSTEM_INPUT) {
	system_event_map_.insert (make_pair (child, pos->second));
      }
    }
  }

  // Unmap the text.
  text->unmap ();

  // Switch back.
  vm::switch_to_directory (original_directory);

  text->override (begin, end);

  if (parse_result == 0) {
    return make_pair (child, LILY_ERROR_SUCCESS);
  }
  else {
    return make_pair (shared_ptr<automaton> (), LILY_ERROR_INVAL);
  }
}

pair<bid_t, lily_error_t>
automaton::bind (const shared_ptr<automaton>& ths,
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
    return make_pair (-1, LILY_ERROR_OAIDDNE);
  }
  
  aid_to_automaton_map_type::const_iterator input_pos = aid_to_automaton_map_.find (input_aid);
  if (input_pos == aid_to_automaton_map_.end ()) {
    // Input automaton DNE.
    return make_pair (-1, LILY_ERROR_IAIDDNE);
  }
  
  shared_ptr<automaton> output_automaton = output_pos->second;
  
  shared_ptr<automaton> input_automaton = input_pos->second;
  
  if (output_automaton == input_automaton) {
    // The output and input automata must be different.
    return make_pair (-1, LILY_ERROR_INVAL);
  }
  
  // Check the output action dynamically.
  const paction* output_action = output_automaton->find_action (output_ano);
  if (output_action == 0 ||
      output_action->type != OUTPUT) {
    // Output action does not exist or has the wrong type.
    return make_pair (-1, LILY_ERROR_OANODNE);
  }
  
  // Check the input action dynamically.
  const paction* input_action = input_automaton->find_action (input_ano);
  if (input_action == 0 ||
      input_action->type != INPUT) {
    // Input action does not exist or has the wrong type.
    return make_pair (-1, LILY_ERROR_IANODNE);
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
  
  {
    // Check that the input action is not bound to an enabled action.
    // (Also checks that the binding does not exist.)
    bound_inputs_map_type::const_iterator pos1 = input_automaton->bound_inputs_map_.find (ia);
    if (pos1 != input_automaton->bound_inputs_map_.end ()) {
      for (binding_set_type::const_iterator pos2 = pos1->second.begin (); pos2 != pos1->second.end (); ++pos2) {
	if ((*pos2)->enabled ()) {
	  // The input is bound to an enabled action.
	  return make_pair (-1, LILY_ERROR_INVAL);
	}
      }
    }
  }
  
  {
    // Check that the output action is not bound to an enabled action in the input automaton.
    bound_outputs_map_type::const_iterator pos1 = output_automaton->bound_outputs_map_.find (oa);
    if (pos1 != output_automaton->bound_outputs_map_.end ()) {
      for (binding_set_type::const_iterator pos2 = pos1->second.begin (); pos2 != pos1->second.end (); ++pos2) {
	if ((*pos2)->enabled () && (*pos2)->input_action.automaton == input_automaton) {
	  return make_pair (-1, LILY_ERROR_INVAL);
	}
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

  // Schedule the output.
  scheduler::schedule (oa);
  
  kout << "TODO:  Generate bind event" << endl;

  return make_pair (b->bid, LILY_ERROR_SUCCESS);
}

void
automaton::unbind (const shared_ptr<binding>& binding,
		   bool remove_from_output,
		   bool remove_from_input,
		   bool remove_from_owner)
{
  kout << "TODO:  Generate unbind event" << endl;

  // Remove from the map.
  size_t count = bid_to_binding_map_.erase (binding->bid);
  kassert (count == 1);
  
  // Disable the binding.
  binding->bid = -1;
  
  // Unbind.
  if (remove_from_output) {
    shared_ptr<automaton> output_automaton = binding->output_action.automaton;
    bound_outputs_map_type::iterator pos = output_automaton->bound_outputs_map_.find (binding->output_action);
    kassert (pos != output_automaton->bound_outputs_map_.end ());
    size_t count = pos->second.erase (binding);
    kassert (count == 1);
    if (pos->second.empty ()) {
      output_automaton->bound_outputs_map_.erase (pos);
    }
  }
  
  if (remove_from_input) {
    shared_ptr<automaton> input_automaton = binding->input_action.automaton;
    bound_inputs_map_type::iterator pos = input_automaton->bound_inputs_map_.find (binding->input_action);
    kassert (pos != input_automaton->bound_inputs_map_.end ());
    size_t count = pos->second.erase (binding);
    kassert (count == 1);
    if (pos->second.empty ()) {
      input_automaton->bound_inputs_map_.erase (pos);
    }
  }
  
  if (remove_from_owner) {
    size_t count = binding->owner->owned_bindings_.erase (binding);
    kassert (count == 1);
  }
  
  for (binding::subscribers_type::const_iterator pos = binding->subscribers.begin ();
       pos != binding->subscribers.end ();
       ++pos) {
    scheduler::schedule (pos->second);
    size_t count = pos->first->binding_subscriptions_.erase (binding);
    kassert (count == 1);
  }
  binding->subscribers.clear ();
}

void
automaton::destroy (const shared_ptr<automaton>& ths)
{
  kassert (ths.get () == this);
  
  kout << "TODO:  Generate destroy/exit event" << endl;

  /*
    Address the instance variables:
    
    aid_
    name_
    ano_to_action_map_
    description_
    regenerate_description_
    parent_
    children_
    execution_mutex_
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
  
  aid_to_automaton_map_.erase (aid_);
  log_event_map_.erase (ths);
  system_event_map_.erase (ths);
  aid_ = -1;
  
  // Leave ano_to_action_map_ for dtor.
  // Leave description_ for dtor.
  // Leave regenterate_description_ for dtor.
  
  parent_ = shared_ptr<automaton> ();
  
  for (children_set_type::const_iterator pos = children_.begin ();
       pos != children_.end ();
       ++pos) {
    (*pos)->destroy (*pos);
  }
  children_.clear ();
  
  // Leave execution_mutex_ for dtor.
  // Leave page_directory_ for dtor.
  // Leave memory_map_ for dtor.
  // Leave heap_area_ for dtor.
  // Leave stack_area_ for dtor.
  // Leave current_bd_ for dtor.
  // Leave bd_to_buffer_map_ for dtor.
  
  for (bound_outputs_map_type::const_iterator pos1 = bound_outputs_map_.begin ();
       pos1 != bound_outputs_map_.end ();
       ++pos1) {
    for (binding_set_type::const_iterator pos2 = pos1->second.begin ();
	 pos2 != pos1->second.end ();
	 ++pos2) {
      unbind (*pos2, false, true, true);
    }
  }
  bound_outputs_map_.clear ();
  
  for (bound_inputs_map_type::const_iterator pos1 = bound_inputs_map_.begin ();
       pos1 != bound_inputs_map_.end ();
       ++pos1) {
    for (binding_set_type::const_iterator pos2 = pos1->second.begin ();
	 pos2 != pos1->second.end ();
	 ++pos2) {
      unbind (*pos2, true, false, true);
    }
  }
  bound_inputs_map_.clear ();
  
  for (binding_set_type::const_iterator pos = owned_bindings_.begin ();
       pos != owned_bindings_.end ();
       ++pos) {
    unbind (*pos, true, true, false);
  }
  owned_bindings_.clear ();
  
  // Leave privileged_ for dtor.
  
  for (mapped_areas_type::const_iterator pos = mapped_areas_.begin ();
       pos != mapped_areas_.end ();
       ++pos) {
    mapped_areas_type::iterator pos3 = find (all_mapped_areas_.begin (), all_mapped_areas_.end (), *pos);
    kassert (pos3 != all_mapped_areas_.end ());
    all_mapped_areas_.erase (pos3);
    memory_map_type::iterator pos2 = find (memory_map_.begin (), memory_map_.end (), *pos);
    kassert (pos2 != memory_map_.end ());
    memory_map_.erase (pos2);
    delete (*pos);
  }
  mapped_areas_.clear ();
  
  for (port_set_type::const_iterator pos = port_set_.begin ();
       pos != port_set_.end ();
       ++pos) {
    reserved_ports_[*pos] = false;
  }
  port_set_.clear ();
  
  for (irq_map_type::const_iterator pos = irq_map_.begin ();
       pos != irq_map_.end ();
       ++pos) {
    irq_handler::unsubscribe (pos->first, pos->second);
  }
  irq_map_.clear ();
  
  kassert (automaton_subscriptions_.empty ());
  
  for (subscribers_type::const_iterator pos = subscribers_.begin ();
       pos != subscribers_.end ();
       ++pos) {
    scheduler::schedule (pos->second);
    size_t count = pos->first->automaton_subscriptions_.erase (ths);
    kassert (count == 1);
  }
  subscribers_.clear ();
  
  kassert (binding_subscriptions_.empty ());
  
  scheduler::remove_automaton (ths);
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

  // pair<size_t, int>
  // buffer_append (bd_t dst,
  // 		 bd_t src,
  // 		 size_t begin,
  // 		 size_t end)
  // {
  //   if (begin > end) {
  //     return make_pair (-1, LILY_SYSCALL_EINVAL);
  //   }

  //   bd_to_buffer_map_type::const_iterator dst_pos = bd_to_buffer_map_.find (dst);
  //   bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
  //   if (dst_pos == bd_to_buffer_map_.end () ||
  // 	src_pos == bd_to_buffer_map_.end ()) {
  //     // One of the buffers does not exist.
  //     return make_pair (-1, LILY_SYSCALL_EBDDNE);
  //   }

  //   buffer* d = dst_pos->second;
  //   buffer* s = src_pos->second;
  //   if (end > s->size ()) {
  //     // Offset is past end of source.
  //     return make_pair (-1, LILY_SYSCALL_EINVAL);
  //   }
    
  //   if (d->begin () != 0) {
  //     // The destination is mapped.
  //     return make_pair (-1, LILY_SYSCALL_EMAPPED);
  //   }

  //   // Append.
  //   return make_pair (d->append (*s, begin, end), LILY_SYSCALL_ESUCCESS);
  // }

  // int
  // buffer_assign (bd_t dest,
  // 		 size_t dest_begin,
  // 		 bd_t src,
  // 		 size_t src_begin,
  // 		 size_t src_end)
  // {
  //   if (src_begin > src_end) {
  //     // Bad range.
  //     return -1;
  //   }

  //   bd_to_buffer_map_type::const_iterator dest_pos = bd_to_buffer_map_.find (dest);
  //   bd_to_buffer_map_type::const_iterator src_pos = bd_to_buffer_map_.find (src);
  //   if (dest_pos == bd_to_buffer_map_.end () ||
  // 	src_pos == bd_to_buffer_map_.end ()) {
  //     // One of the buffers does not exist.
  //     return -1;
  //   }

  //   buffer* dest_b = dest_pos->second;
  //   buffer* src_b = src_pos->second;

  //   if (src_end > src_b->size ()) {
  //     return -1;
  //   }

  //   if (dest_begin + (src_end - src_begin) > dest_b->size ()) {
  //     return -1;
  //   }

  //   // Assign.
  //   dest_b->assign (dest_begin, *src_b, src_begin, src_end);
  //   return 0;
  // }
