#include "automaton.hpp"
#include "scheduler.hpp"
#include "elf.hpp"

aid_t automaton::current_aid_ = 0;
automaton::aid_to_automaton_map_type automaton::aid_to_automaton_map_;
automaton::registry_map_type automaton::registry_map_;

bid_t automaton::current_bid_ = 0;
automaton::bid_to_binding_map_type automaton::bid_to_binding_map_;

bitset<ONE_MEGABYTE / PAGE_SIZE> automaton::mmapped_frames_;
bitset<65536> automaton::reserved_ports_;

void
automaton::finish (const shared_ptr<automaton>& ths,
		   ano_t action_number,
		   int parameter,
		   bool output_fired,
		   bd_t bda,
		   bd_t bdb)
{
  kassert (ths.get () == this);

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
	scheduler::schedule (caction (ths, action, parameter));
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

pair<shared_ptr<automaton>, int>
automaton::create_automaton (const kstring& name,
			     const shared_ptr<automaton>& parent,
			     bool privileged,
			     buffer* text,
			     size_t text_size,
			     buffer* buffer_a,
			     buffer* buffer_b)
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
  if (elf::parse (child, text_begin, text_end) == -1) {
    return make_pair (shared_ptr<automaton> (), LILY_SYSCALL_EBADTEXT);
  }

  // Unmap the text.
  text->unmap ();

  // Generate an id and insert into the aid to automaton map.
  aid_t child_aid = current_aid_;
  while (aid_to_automaton_map_.find (child_aid) != aid_to_automaton_map_.end ()) {
    child_aid = max (child_aid + 1, 0); // Handles overflow.
  }
  current_aid_ = max (child_aid + 1, 0);

  aid_to_automaton_map_.insert (make_pair (child_aid, child));

  // Insert into the name map if necessary.
  if (name.size () != 0) {
    pair<registry_map_type::iterator, bool> r = registry_map_.insert (make_pair (name, child));
    kassert (r.second);
  }

  // Add the child to the parent.
  if (parent.get () != 0) {
    parent->children_.insert (child);
  }

  // Switch back.
  vm::switch_to_directory (original_directory);
  
  text->override (begin, end);

  child->aid_ = child_aid;
  child->name_ = name;
  child->parent_ = parent;
  child->privileged_ = privileged;

  // Add to the scheduler.
  scheduler::add_automaton (child);

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
    
    scheduler::schedule (caction (child, action, child_aid, buffer_a, buffer_b));
  }

  return make_pair (child, LILY_SYSCALL_ESUCCESS);    
}

pair<int, int>
automaton::unbind (const shared_ptr<automaton>& ths,
		   bid_t bid)
{
  kassert (ths.get () == this);
  
  // Look up the binding.
  bid_to_binding_map_type::iterator pos = bid_to_binding_map_.find (bid);
  if (pos == bid_to_binding_map_.end ()) {
    return make_pair (-1, LILY_SYSCALL_EBIDDNE);
  }
  
  shared_ptr<binding> binding = pos->second;
  
  // Are we the owner?
  if (binding->owner != ths) {
    return make_pair (-1, LILY_SYSCALL_ENOTOWNER);
  }
  
  // Disable the binding.
  binding->bid = -1;
  
  // Remove from the map.
  bid_to_binding_map_.erase (pos);
  
  // Unbind.
  {
    size_t count = owned_bindings_.erase (binding);
    kassert (count == 1);
  }
  
  {
    shared_ptr<automaton> input_automaton = binding->input_action.automaton;
    bound_inputs_map_type::iterator pos = input_automaton->bound_inputs_map_.find (binding->input_action);
    kassert (pos != input_automaton->bound_inputs_map_.end ());
    size_t count = pos->second.erase (binding);
    kassert (count == 1);
    if (pos->second.empty ()) {
      input_automaton->bound_inputs_map_.erase (pos);
    }
  }
  
  {
    shared_ptr<automaton> output_automaton = binding->output_action.automaton;
    bound_outputs_map_type::iterator pos = output_automaton->bound_outputs_map_.find (binding->output_action);
    kassert (pos != output_automaton->bound_outputs_map_.end ());
    size_t count = pos->second.erase (binding);
    kassert (count == 1);
    if (pos->second.empty ()) {
      output_automaton->bound_outputs_map_.erase (pos);
    }
  }
  
  // BUG:  Do something with subscribers.    
  for (binding::subscribers_type::const_iterator pos = binding->subscribers.begin ();
       pos != binding->subscribers.end ();
       ++pos) {
    scheduler::schedule (pos->second);
    size_t count = pos->first->binding_subscriptions_.erase (binding);
    kassert (count == 1);
  }
  binding->subscribers.clear ();
  
  return make_pair (0, LILY_SYSCALL_ESUCCESS);
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

  // children_set_type::const_iterator
  // children_begin () const
  // {
  //   return children_.begin ();
  // }

  // children_set_type::const_iterator
  // children_end () const
  // {
  //   return children_.end ();
  // }

  // automaton*
  // forget_parent (void)
  // {
  //   for (children_set_type::const_iterator pos = children_.begin ();
  // 	 pos != children_.end ();
  // 	 ++pos) {
  //     (*pos)->forget_parent ();
  //   }

  //   if (parent_ != 0) {
  //     parent_->decref ();
  //   }
  //   automaton* retval = parent_;
  //   parent_ = 0;
  //   return retval;
  // }

  // void
  // forget_child (automaton* child)
  // {
  //   size_t count = children_.erase (child);
  //   kassert (count == 1);
  //   child->decref ();
  // }

  // void
  // forget_children (void)
  // {
  //   for (children_set_type::const_iterator pos = children_.begin ();
  // 	 pos != children_.end ();
  // 	 ++pos) {
  //     (*pos)->forget_children ();
  //     (*pos)->decref ();
  //   }

  //   children_.clear ();
  // }

  // void
  // disable ()
  // {
  //   if (enabled_) {
  //     // Disable ourselves.
  //     enabled_ = false;
      
  //     // Disable our bindings.
  //     for (bound_outputs_map_type::const_iterator pos = bound_outputs_map_.begin ();
  // 	   pos != bound_outputs_map_.end ();
  // 	   ++pos) {
  // 	for (binding_set_type::const_iterator pos1 = pos->second.begin ();
  // 	     pos1 != pos->second.end ();
  // 	     ++pos1) {
  // 	  (*pos1)->disable ();
  // 	}
  //     }
  //     for (bound_inputs_map_type::const_iterator pos = bound_inputs_map_.begin ();
  // 	   pos != bound_inputs_map_.end ();
  // 	   ++pos) {
  // 	for (binding_set_type::const_iterator pos1 = pos->second.begin ();
  // 	     pos1 != pos->second.end ();
  // 	     ++pos1) {
  // 	  (*pos1)->disable ();
  // 	}
  //     }
  //     for (binding_set_type::const_iterator pos1 = owned_bindings_.begin ();
  // 	   pos1 != owned_bindings_.end ();
  // 	   ++pos1) {
  // 	(*pos1)->disable ();
  //     }
      
  //     // Disable our children.
  //     for (children_set_type::const_iterator pos = children_.begin ();
  // 	   pos != children_.end ();
  // 	   ++pos) {
  // 	(*pos)->disable ();
  //     }
  //   }
  // }

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

  // void
  // purge_bindings (void)
  // {
  //   // Copy the bindings.
  //   binding_set_type all_bindings;
  //   for (bound_outputs_map_type::const_iterator pos = bound_outputs_map_.begin ();
  //   	 pos != bound_outputs_map_.end ();
  //   	 ++pos) {
  //     for (binding_set_type::const_iterator pos1 = pos->second.begin ();
  //   	   pos1 != pos->second.end ();
  //   	   ++pos1) {
  // 	all_bindings.insert (*pos1);
  //     }
  //   }
  //   for (bound_inputs_map_type::const_iterator pos = bound_inputs_map_.begin ();
  //   	 pos != bound_inputs_map_.end ();
  //   	 ++pos) {
  //     for (binding_set_type::const_iterator pos1 = pos->second.begin ();
  //   	   pos1 != pos->second.end ();
  //   	   ++pos1) {
  //   	all_bindings.insert (*pos1);
  //     }
  //   }
  //   for (binding_set_type::const_iterator pos1 = owned_bindings_.begin ();
  //   	 pos1 != owned_bindings_.end ();
  //   	 ++pos1) {
  //     all_bindings.insert (*pos1);
  //   }

  //   for (binding_set_type::const_iterator pos = all_bindings.begin ();
  // 	 pos != all_bindings.end ();
  // 	 ++pos) {
  //     binding* b = *pos;
  //     shared_ptr<automaton> output_automaton = b->output_action ().automaton;
  //     shared_ptr<automaton> input_automaton = b->input_action ().automaton;
  //     shared_ptr<automaton> owner = b->owner ();
  //     lock_bindings_in_order (output_automaton, input_automaton, owner);
  //     output_automaton->unbind_output (b);
  //     input_automaton->unbind_input (b);
  //     owner->unbind (b);
  //     unlock_bindings_in_order (output_automaton, input_automaton, owner);
  //   }

  //   kassert (bound_outputs_map_.empty ());
  //   kassert (bound_inputs_map_.empty ());
  //   kassert (owned_bindings_.empty ());

  //   for (children_set_type::const_iterator pos = children_.begin ();
  // 	 pos != children_.end ();
  // 	 ++pos) {
  //     (*pos)->purge_bindings ();
  //   }
  // }

  // inline void
  // unbind_output (const shared_ptr<binding>& b)
  // {
  //   kassert (b->output_action ().automaton.get () == this);

  //   bound_outputs_map_type::iterator pos = bound_outputs_map_.find (b->output_action ());
  //   kassert (pos != bound_outputs_map_.end ());
  //   size_t count = pos->second.erase (b);
  //   kassert (count == 1);
  //   if (pos->second.empty ()) {
  //     bound_outputs_map_.erase (pos);
  //   }
  // }

  // inline void
  // unbind_input (const shared_ptr<binding>& b)
  // {
  //   kassert (b->input_action ().automaton.get () == this);

  //   bound_inputs_map_type::iterator pos = bound_inputs_map_.find (b->input_action ());
  //   kassert (pos != bound_inputs_map_.end ());
  //   size_t count = pos->second.erase (b);
  //   kassert (count == 1);
  //   if (pos->second.empty ()) {
  //     bound_inputs_map_.erase (pos);
  //   }
  // }

  // inline void
  // unbind (const shared_ptr<binding>& b)
  // {
  //   size_t count = owned_bindings_.erase (b);
  //   kassert (count == 1);
  // }
