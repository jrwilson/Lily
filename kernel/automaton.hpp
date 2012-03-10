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

// The stack.
static const logical_address_t STACK_END = KERNEL_VIRTUAL_BASE;
static const logical_address_t STACK_BEGIN = STACK_END - PAGE_SIZE;

class automaton {
private:
  // Next binding id to allocate.
  static aid_t current_aid_;

  // Map from aid to automaton.
  typedef unordered_map<aid_t, automaton*> aid_to_automaton_map_type;
  static aid_to_automaton_map_type aid_to_automaton_map_;

  static aid_t
  generate_aid (automaton* a)
  {
    // TODO:  This needs to be atomic.

    // Generate an id.
    aid_t aid = current_aid_;
    while (aid_to_automaton_map_.find (aid) != aid_to_automaton_map_.end ()) {
      aid = max (aid + 1, 0); // Handles overflow.
    }
    current_aid_ = max (aid + 1, 0);

    aid_to_automaton_map_.insert (make_pair (aid, a));

    return aid;
  }
public:

  static inline automaton*
  lookup (aid_t aid)
  {
    // TODO:  This should lookup and increment the reference count atomically.
    aid_to_automaton_map_type::const_iterator pos = aid_to_automaton_map_.find (aid);
    if (pos != aid_to_automaton_map_.end ()) {
      return pos->second;
    }
    else {
      return 0;
    }
  }

private:
  automaton* parent_;
public:
  typedef unordered_set<automaton*> children_set_type;
private:
  children_set_type children_;
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
  typedef unordered_set <binding*> binding_set_type;
private:
  typedef unordered_map <caction, binding_set_type, caction_hash> bound_outputs_map_type;
  bound_outputs_map_type bound_outputs_map_;

  // Bound inputs.
  typedef unordered_map<caction, binding_set_type, caction_hash> bound_inputs_map_type;
  bound_inputs_map_type bound_inputs_map_;

  // Owned bindings.
  binding_set_type owned_bindings_;

  binding_set_type empty_set_;

  // Next buffer descriptor to allocate.
  bd_t current_bd_;
  // Map from bd_t to buffer*.
  typedef unordered_map<bd_t, buffer*> bd_to_buffer_map_type;
  bd_to_buffer_map_type bd_to_buffer_map_;

public:
  // Reference count.
  size_t refcount_;
private:
  // Mutual exlusion lock for executing actions.
  int exec_lock_;
  // Mutual exclusion lock for manipulating the bindings associated with this automaton.
  int binding_lock_;
  // Flag indicating if this automaton can execute actions.
  bool enabled_;

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

  static void
  lock_bindings_in_order (automaton* a,
			  automaton* b,
			  automaton* c)
  {
    // TODO
  }

  static void
  unlock_bindings_in_order (automaton* a,
			    automaton* b,
			    automaton* c)
  {
    // TODO
  }

  ~automaton ()
  {
    // BUG:  Destroy vm_areas, buffers, etc.
    kassert (0);
  }

public:

  automaton (automaton* parent,
	     bool privileged,
	     frame_t page_directory_frame) :
    parent_ (parent),
    aid_ (generate_aid (this)),
    privileged_ (privileged),
    page_directory_ (frame_to_physical_address (page_directory_frame)),
    heap_area_ (0),
    stack_area_ (0),
    current_bd_ (0),
    refcount_ (1),
    exec_lock_ (0),
    binding_lock_ (0),
    enabled_ (true)
  {
    if (parent_ != 0) {
      parent_->incref ();
    }
  }

  /* These methods are used when construction the automaton.
     Some of them should probably be changed to constructor arguments. */
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
      vm::map (address, frame, vm::USER, vm::MAP_READ_WRITE);
    }
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

  /* This is used during creation and execution. */
  physical_address_t
  page_directory_physical_address () const
  {
    return page_directory_;
  }

  /* These methods are for querying static information about the automaton. */
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

  /* Reference counting. */
  void
  incref ()
  {
    // TODO:  This needs to be atomic.
    ++refcount_;
  }

  void
  decref ()
  {
    // TODO:  This needs to be atomic.
    --refcount_;
    if (refcount_ == 0) {
      delete this;
    }
  }

  bool
  enabled () const
  {
    return enabled_;
  }

  void
  disable ()
  {
    // TODO:  This should be atomic.
    // Disable ourselves.
    enabled_ = false;

    // Disable our bindings.
    for (bound_outputs_map_type::const_iterator pos = bound_outputs_map_.begin ();
  	 pos != bound_outputs_map_.end ();
  	 ++pos) {
      for (binding_set_type::const_iterator pos1 = pos->second.begin ();
  	   pos1 != pos->second.end ();
  	   ++pos1) {
  	(*pos1)->disable ();
      }
    }
    for (bound_inputs_map_type::const_iterator pos = bound_inputs_map_.begin ();
  	 pos != bound_inputs_map_.end ();
  	 ++pos) {
      for (binding_set_type::const_iterator pos1 = pos->second.begin ();
  	   pos1 != pos->second.end ();
  	   ++pos1) {
  	(*pos1)->disable ();
      }
    }
    for (binding_set_type::const_iterator pos1 = owned_bindings_.begin ();
  	 pos1 != owned_bindings_.end ();
  	 ++pos1) {
      (*pos1)->disable ();
    }

    // Disable our children.
    for (children_set_type::const_iterator pos = children_.begin ();
  	 pos != children_.end ();
  	 ++pos) {
      (*pos)->disable ();
    }
  }

  automaton*
  forget_parent (void)
  {
    // TODO:  This should be atomic (?)
    for (children_set_type::const_iterator pos = children_.begin ();
  	 pos != children_.end ();
  	 ++pos) {
      (*pos)->forget_parent ();
    }

    if (parent_ != 0) {
      parent_->decref ();
    }
    automaton* retval = parent_;
    parent_ = 0;
    return retval;
  }
  
  children_set_type::const_iterator
  children_begin () const
  {
    return children_.begin ();
  }

  children_set_type::const_iterator
  children_end () const
  {
    return children_.end ();
  }

  void
  forget_child (automaton* child)
  {
    // TODO:  This should be atomic.
    size_t count = children_.erase (child);
    kassert (count == 1);
    child->decref ();
  }

  void
  purge_bindings (void)
  {
    // Copy the bindings.
    binding_set_type all_bindings;
    for (bound_outputs_map_type::const_iterator pos = bound_outputs_map_.begin ();
    	 pos != bound_outputs_map_.end ();
    	 ++pos) {
      for (binding_set_type::const_iterator pos1 = pos->second.begin ();
    	   pos1 != pos->second.end ();
    	   ++pos1) {
  	all_bindings.insert (*pos1);
      }
    }
    for (bound_inputs_map_type::const_iterator pos = bound_inputs_map_.begin ();
    	 pos != bound_inputs_map_.end ();
    	 ++pos) {
      for (binding_set_type::const_iterator pos1 = pos->second.begin ();
    	   pos1 != pos->second.end ();
    	   ++pos1) {
    	all_bindings.insert (*pos1);
      }
    }
    for (binding_set_type::const_iterator pos1 = owned_bindings_.begin ();
    	 pos1 != owned_bindings_.end ();
    	 ++pos1) {
      all_bindings.insert (*pos1);
    }

    for (binding_set_type::const_iterator pos = all_bindings.begin ();
  	 pos != all_bindings.end ();
  	 ++pos) {
      binding* b = *pos;
      automaton* output_automaton = b->output_action ().action->automaton;
      automaton* input_automaton = b->input_action ().action->automaton;
      automaton* owner = b->owner ();
      lock_bindings_in_order (output_automaton, input_automaton, owner);
      output_automaton->unbind_output (b);
      input_automaton->unbind_input (b);
      owner->unbind (b);
      unlock_bindings_in_order (output_automaton, input_automaton, owner);
    }

    kassert (bound_outputs_map_.empty ());
    kassert (bound_inputs_map_.empty ());
    kassert (owned_bindings_.empty ());

    for (children_set_type::const_iterator pos = children_.begin ();
  	 pos != children_.end ();
  	 ++pos) {
      (*pos)->purge_bindings ();
    }
  }

  void
  forget_children (void)
  {
    for (children_set_type::const_iterator pos = children_.begin ();
  	 pos != children_.end ();
  	 ++pos) {
      (*pos)->forget_children ();
      (*pos)->decref ();
    }

    children_.clear ();
  }

  void
  add_subscriber (automaton* a)
  {
    // TODO:  This needs to be atomic.
    pair<subscribers_set_type::iterator, bool> r = subscribers_.insert (a);
    if (r.second) {
      a->incref ();
    }
  }
  
  /******************************************************************************************
   * The functions between lock_bindings and unlock_bindings all require the bindings lock. *
   ******************************************************************************************/

  void
  lock_bindings ()
  {
    // TODO
  }

  bool
  is_output_bound_to_automaton (const caction& output_action,
				const automaton* input_automaton) const
  {
    kassert (output_action.action->automaton == this);

    bound_outputs_map_type::const_iterator pos1 = bound_outputs_map_.find (output_action);
    if (pos1 != bound_outputs_map_.end ()) {
      // TODO:  Replace this iteration with look-up in a data structure.
      for (binding_set_type::const_iterator pos2 = pos1->second.begin (); pos2 != pos1->second.end (); ++pos2) {
	if ((*pos2)->input_action ().action->automaton == input_automaton) {
	  return true;
	}
      }
    }

    return false;
  }

  void
  bind_output (binding* b)
  {
    
    kassert (b->output_action ().action->automaton == this);

    pair<bound_outputs_map_type::iterator, bool> r = bound_outputs_map_.insert (make_pair (b->output_action (), binding_set_type ()));
    r.first->second.insert (b);
    b->incref ();
  }

  void
  unbind_output (binding* b)
  {
    kassert (b->output_action ().action->automaton == this);

    bound_outputs_map_type::iterator pos = bound_outputs_map_.find (b->output_action ());
    kassert (pos != bound_outputs_map_.end ());
    size_t count = pos->second.erase (b);
    b->decref ();
    kassert (count == 1);
    if (pos->second.empty ()) {
      bound_outputs_map_.erase (pos);
    }
  }
  
  bool
  is_input_bound (const caction& input_action) const
  {
    kassert (input_action.action->automaton == this);

    return bound_inputs_map_.find (input_action) != bound_inputs_map_.end ();
  }

  void
  bind_input (binding* b)
  {
    
    kassert (b->input_action ().action->automaton == this);

    pair<bound_inputs_map_type::iterator, bool> r = bound_inputs_map_.insert (make_pair (b->input_action (), binding_set_type ()));
    r.first->second.insert (b);
    b->incref ();
  }

  void
  unbind_input (binding* b)
  {
    kassert (b->input_action ().action->automaton == this);

    bound_inputs_map_type::iterator pos = bound_inputs_map_.find (b->input_action ());
    kassert (pos != bound_inputs_map_.end ());
    size_t count = pos->second.erase (b);
    b->decref ();
    kassert (count == 1);
    if (pos->second.empty ()) {
      bound_inputs_map_.erase (pos);
    }
  }

  void
  bind (binding* b)
  {
    
    pair <binding_set_type::iterator, bool> r = owned_bindings_.insert (b);
    kassert (r.second);
    b->incref ();
  }

  void
  unbind (binding* b)
  {
    size_t count = owned_bindings_.erase (b);
    kassert (count == 1);
    b->decref ();
  }

  const binding_set_type&
  get_bound_inputs (const caction& output_action) const
  {
    bound_outputs_map_type::const_iterator pos = bound_outputs_map_.find (output_action);
    if (pos != bound_outputs_map_.end ()) {
      return pos->second;
    }
    else {
      return empty_set_;
    }
  }

  void
  unlock_bindings ()
  {
    // TODO
  }

  /*********************************************************************************************
   * The functions between lock_execution and unlock_execution all require the execution lock. *
   *********************************************************************************************/

  void
  lock_execution ()
  {
    // TODO
  }

  void
  add_child (automaton* child)
  {
    // TODO:  This should be atomic.
    pair<children_set_type::iterator, bool> r = children_.insert (child);
    kassert (r.second);
    child->incref ();
  }

  logical_address_t
  stack_pointer () const
  {
    return stack_area_->end ();
  }

  bool
  verify_span (const void* ptr,
  	       size_t size) const
  {
    const logical_address_t address = reinterpret_cast<logical_address_t> (ptr);
    memory_map_type::const_iterator pos = find_address (address);
    return pos != memory_map_.end () && (*pos)->begin () <= address && (address + size) <= (*pos)->end ();
  }

  bool
  verify_stack (const void* ptr,
		size_t size) const
  {
    
    kassert (stack_area_ != 0);
    const logical_address_t address = reinterpret_cast<logical_address_t> (ptr);
    return stack_area_->begin () <= address && (address + size) <= stack_area_->end ();
  }

  void
  add_subscription (automaton* a)
  {
    
    pair<subscribers_set_type::iterator, bool> r = subscriptions_.insert (a);
    if (r.second) {
      a->incref ();
    }
  }

  // BUG:  Limit the number and size of buffers.

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

  pair<int, int>
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

  pair<uint8_t, int>
  inb (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inb (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  pair<int, int>
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

  pair<uint16_t, int>
  inw (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inw (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  pair<int, int>
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

  pair<uint32_t, int>
  inl (uint16_t port)
  {
    if (port_set_.find (port) != port_set_.end ()) {
      return make_pair (io::inl (port), LILY_SYSCALL_ESUCCESS);
    }
    else {
      return make_pair (-1, LILY_SYSCALL_EPERM);
    }
  }

  pair<int, int>
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

  void
  unlock_execution ()
  {
    // TODO
  }

};

#endif /* __automaton_hpp__ */
