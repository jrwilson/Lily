#include "automaton.hpp"
#include "scheduler.hpp"
#include "elf.hpp"

aid_t automaton::current_aid_ = 0;
automaton::aid_to_automaton_map_type automaton::aid_to_automaton_map_;
automaton::registry_map_type automaton::registry_map_;
mutex automaton::id_mutex_;

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
 
template <typename T>
class auto_decref {
private:
  T& obj_;
public:
  auto_decref (T& obj) :
    obj_ (obj)
  { }

  ~auto_decref ()
  {
    obj_.decref ();
  }
};

pair<automaton*, int>
automaton::create_automaton (const kstring& name,
			     automaton* parent,
			     bool privileged,
			     buffer* text,
			     size_t text_size,
			     buffer* buffer_a,
			     buffer* buffer_b)
{
  // Create the automaton.
  automaton* child = new automaton ();

  // We want to drop its reference count when we return.
  // To succeed we must increment its reference count.
  auto_decref<automaton> child_cleanup (*child);

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
    return make_pair ((automaton*)0, LILY_SYSCALL_EBADTEXT);
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

  if (parent != 0) {
    // Acquire the modification mutex for the parent.
    lock plock (parent->mod_mutex_);
    parent->children_.insert (child);
    // TODO:  Is the locking correct?
    child->incref ();
  }

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
    
    scheduler::schedule (caction (action, child_aid, buffer_a, buffer_b));
  }

  // Switch back.
  vm::switch_to_directory (original_directory);
  
  text->override (begin, end);

  child->aid_ = child_aid;
  child->name_ = name;
  child->parent_ = parent;
  if (parent != 0) {
    // TODO:  Is the locking correct?
    parent->incref ();
  }
  child->privileged_ = privileged;

  // Increment the reference count for success.
  child->incref ();

  return make_pair (child, LILY_SYSCALL_ESUCCESS);    
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
  lock id_lock (id_mutex_);

  // Check the name data.
  if (name_size != 0 && (!verify_span (name, name_size) || name[name_size - 1] != 0)) {
    return make_pair (-1, LILY_SYSCALL_EINVAL);
  }

  // Check that the name does not exist.
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

  // Find and synchronize the data buffers so the frames listed in the buffers are correct.
  buffer* buffer_a = lookup_buffer (bda);
  if (buffer_a != 0) {
    buffer_a->sync (0, buffer_a->size ());
  }
  buffer* buffer_b = lookup_buffer (bdb);
  if (buffer_b != 0) {
    buffer_b->sync (0, buffer_b->size ());
  }
  
  // Create the automaton.
  pair<automaton*, int> r = create_automaton (name_str, this, retain_privilege && privileged_, text_buffer, text_buffer->size () * PAGE_SIZE, buffer_a, buffer_b);

  if (r.first != 0) {
    return make_pair (r.first->aid (), r.second);
  }
  else {
    return make_pair (-1, r.second);
  }
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
    vm::switch_to_directory (page_directory);
    
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
