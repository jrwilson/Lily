#include "automaton.hpp"
#include "scheduler.hpp"
#include "elf.hpp"

aid_t automaton::current_aid_ = 0;
automaton::aid_to_automaton_map_type automaton::aid_to_automaton_map_;

bid_t automaton::current_bid_ = 0;
automaton::bid_to_binding_map_type automaton::bid_to_binding_map_;

automaton::mapped_areas_type automaton::all_mapped_areas_;
bitset<65536> automaton::reserved_ports_;

// automaton::log_event_map_type automaton::log_event_map_;

pair<int, lily_error_t>
automaton::schedule (const shared_ptr<automaton>& ths,
		     ano_t action_number,
		     int parameter)
{
  kassert (ths.get () == this);

  const paction* action = find_action (action_number);
  if (action != 0) {
    /* Check the parameter. */
    if (action->parameter_mode == NO_PARAMETER && parameter != 0) {
      return make_pair (-1, LILY_ERROR_INVAL);
    }
    
    switch (action->type) {
    case OUTPUT:
    case INTERNAL:
    case SYSTEM:
      scheduler::schedule (caction (ths, action, parameter));
      return make_pair (0, LILY_ERROR_SUCCESS);
      break;
    case INPUT:
      return make_pair (-1, LILY_ERROR_INVAL);
      break;
    }
  }

  return make_pair (-1, LILY_ERROR_ANODNE);
}

// The automaton would like to no longer exist.
void
automaton::exit (const shared_ptr<automaton>& ths,
		 int code)
{
  kassert (ths.get () == this);
      
  destroy (ths);
  
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
automaton::create_automaton (bool privileged,
			     const shared_ptr<buffer>& text,
			     size_t text_size)
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
    
    child->aid_ = child_aid;
    child->privileged_ = privileged;
    
    // Add to the scheduler.
    scheduler::add_automaton (child);
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

void
automaton::destroy (const shared_ptr<automaton>& ths)
{
  kassert (ths.get () == this);
  
  kout << "TODO:  Generate destroy/exit event" << endl;

  enabled_ = false;

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
  // log_event_map_.erase (ths);
  aid_ = -1;
  
  // Leave ano_to_action_map_ for dtor.
  // Leave description_ for dtor.
  // Leave regenterate_description_ for dtor.
  
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
      unbind (*pos2, false, true);
    }
  }
  bound_outputs_map_.clear ();
  
  for (bound_inputs_map_type::const_iterator pos1 = bound_inputs_map_.begin ();
       pos1 != bound_inputs_map_.end ();
       ++pos1) {
    for (binding_set_type::const_iterator pos2 = pos1->second.begin ();
	 pos2 != pos1->second.end ();
	 ++pos2) {
      unbind (*pos2, true, false);
    }
  }
  bound_inputs_map_.clear ();
  
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
