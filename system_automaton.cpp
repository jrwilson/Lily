/*
  File
  ----
  system_automaton.cpp
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "automaton.hpp"
#include "scheduler.hpp"

#include "action_test_automaton.hpp"
#include "buffer_test_automaton.hpp"
#include "ramdisk_automaton.hpp"
#include "ext2_automaton.hpp"

// Symbols to build memory maps.
extern int text_begin;
extern int text_end;
extern int rodata_begin;
extern int rodata_end;
extern int data_begin;
extern int data_end;

namespace system_automaton {

  static list_alloc_state state_;

  struct system_automaton_syscall : public list_alloc_syscall {
    static list_alloc_state&
    state ()
    {
      return state_;
    }
  };

  typedef list_alloc<system_automaton_syscall> alloc_type;

  template <typename T>
  struct allocator_type : public list_allocator<T, system_automaton_syscall> { };

  // Heap location for system automata.
  // Starting at 4MB allow us to use the low page table of the kernel.
  static const logical_address_t SYSTEM_HEAP_BEGIN = FOUR_MEGABYTES;
  
  // Stack range for system automata.
  static const logical_address_t SYSTEM_STACK_BEGIN = KERNEL_VIRTUAL_BASE - PAGE_SIZE;
  static const logical_address_t SYSTEM_STACK_END = KERNEL_VIRTUAL_BASE;

  static physical_address_t initrd_begin_;
  static physical_address_t initrd_end_;

  // Allocated with system allocator.
  static aid_t current_aid_;
  static aid_map_type aid_map_;

  static automaton* system_automaton_ = 0;
  static automaton* output_automaton_ = 0;
  static automaton* input_automaton_ = 0;
  static automaton* buffer_test_automaton_ = 0;
  static automaton* ramdisk_automaton_ = 0;
  static automaton* ext2_automaton_ = 0;

  // Allocated with system automaton allocator.

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  typedef std::deque<automaton*, allocator_type<automaton*> > init_queue_type;
  init_queue_type* init_queue_ = 0;

  static bool bindsys_ = false;

  static automaton*
  create (bool privcall,
	  vm::page_privilege_t map_privilege,
	  vm::page_privilege_t kernel_privilege)
  {
    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    // Map the page directory.
    vm::map (vm::get_stub1 (), frame, vm::USER, vm::WRITABLE);
    vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
    // Initialize the page directory.
    // If the second argument is vm::USER, the automaton can access the paging area, i.e., manipulate virtual memory.
    // If the second argument is vm::SUPERVISOR, the automaton cannot.
    // This is useful for the system automaton because it maps in frames to create page directories by calling this function.
    // However, it is not necessary as it can be done using buffers.
    new (pd) vm::page_directory (frame, map_privilege);
    // Initialize the page directory with a copy of the kernel.
    // If the third argument is vm::USER, the automaton gains access to kernel data.
    // If the third argument is vm::SUPERVISOR, the automaton does not.
    vm::copy_page_directory (pd, vm::get_kernel_page_directory (), kernel_privilege);
    // Unmap.
    vm::unmap (vm::get_stub1 ());
    // Drop the reference count from allocation.
    size_t count = frame_manager::decref (frame);
    kassert (count == 1);
    
    // Generate an id.
    aid_t aid = current_aid_;
    while (aid_map_.find (aid) != aid_map_.end ()) {
      aid = std::max (aid + 1, 0); // Handles overflow.
    }
    current_aid_ = std::max (aid + 1, 0);
    
    // Create the automaton and insert it into the map.
    automaton* a = new (system_alloc ()) automaton (aid, frame, privcall);
    aid_map_.insert (std::make_pair (aid, a));
    
    // Add to the scheduler.
    scheduler::add_automaton (a);
    
    return a;
  }

  static void
  bind_ (automaton* output_automaton,
	 const void* output_action_entry_point,
	 parameter_mode_t output_parameter_mode,
	 aid_t output_parameter,
	 automaton* input_automaton,
	 const void* input_action_entry_point,
	 parameter_mode_t input_parameter_mode,
	 aid_t input_parameter,
	 buffer_value_mode_t buffer_value_mode,
	 copy_value_mode_t copy_value_mode,
	 size_t copy_value_size,
	 automaton* owner)
  {
    // Check the output action dynamically.
    kassert (output_automaton != 0);
    automaton::const_action_iterator output_pos = output_automaton->action_find (output_action_entry_point);
    kassert (output_pos != output_automaton->action_end () &&
	     (*output_pos)->type == OUTPUT &&
	     (*output_pos)->parameter_mode == output_parameter_mode &&
	     (*output_pos)->buffer_value_mode == buffer_value_mode &&
	     (*output_pos)->copy_value_mode == copy_value_mode &&
	     (*output_pos)->copy_value_size == copy_value_size);
    
    // Check the input action dynamically.
    kassert (input_automaton != 0);
    automaton::const_action_iterator input_pos = input_automaton->action_find (input_action_entry_point);
    kassert (input_pos != input_automaton->action_end () &&
	     (*input_pos)->type == INPUT &&
	     (*input_pos)->parameter_mode == input_parameter_mode &&
	     (*input_pos)->buffer_value_mode == buffer_value_mode &&
	     (*input_pos)->copy_value_mode == copy_value_mode &&
	     (*input_pos)->copy_value_size == copy_value_size);
    
    // TODO:  All of the bind checks.
    caction oa (*output_pos, output_parameter);
    caction ia (*input_pos, input_parameter);
    
    output_automaton->bind_output (oa, ia, owner);
    input_automaton->bind_input (oa, ia, owner);
    owner->bind (oa, ia);
  }
  
  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (void),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (void),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>  
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (void),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::copy_value_type),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::copy_value_type),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::copy_value_type),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
		  typename InputAction::parameter_type input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  no_buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  no_copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (no_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (void),
		  auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (typename OutputAction::parameter_type),
		  typename OutputAction::parameter_type output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction, class InputAction>
  static void
  bind_dispatch_ (auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* output_automaton,
		  void (*output_ptr) (aid_t),
		  aid_t output_parameter,
		  auto_parameter_tag,
		  buffer_value_tag,
		  copy_value_tag,
		  automaton* input_automaton,
		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
		  aid_t input_parameter,
		  size_t copy_value_size,
		  automaton* owner)
  {
    bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
     	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (void),
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (void),
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (IT1),
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (IT1),
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (IT1, IT2),
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (IT1, IT2),
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type),
	typename InputAction::parameter_type input_parameter,
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type),
	typename InputAction::parameter_type input_parameter,
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1),
	typename InputAction::parameter_type input_parameter,
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1),
	typename InputAction::parameter_type input_parameter,
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (void),
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1, IT2),
	typename InputAction::parameter_type input_parameter,
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size,
					       owner);
  }

  template <class OutputAction,
	    class InputAction,
	    class IT1,
	    class IT2>
  static void
  bind (automaton* output_automaton,
  	void (*output_ptr) (typename OutputAction::parameter_type),
	typename OutputAction::parameter_type output_parameter,
  	automaton* input_automaton,
  	void (*input_ptr) (typename InputAction::parameter_type, IT1, IT2),
	typename InputAction::parameter_type input_parameter,
	automaton* owner)
  {
    // Check both actions statically.
    STATIC_ASSERT (is_output_action<OutputAction>::value);
    STATIC_ASSERT (is_input_action<InputAction>::value);
    // Value types must be the same.    
    STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
    STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

    bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
					       typename OutputAction::buffer_value_category (),
					       typename OutputAction::copy_value_category (),
					       output_automaton,
					       output_ptr,
					       output_parameter,
					       typename InputAction::parameter_category (),
					       typename InputAction::buffer_value_category (),
					       typename InputAction::copy_value_category (),
					       input_automaton,
					       input_ptr,
					       input_parameter,
					       OutputAction::copy_value_size,
					       owner);
  }

  static void
  checked_schedule (automaton* a,
		    const void* aep,
		    aid_t p = 0)
  {
    automaton::const_action_iterator pos = a->action_find (aep);
    kassert (pos != a->action_end ());
    scheduler::schedule (caction (*pos, p));
  }

  const_automaton_iterator
  automaton_begin ()
  {
    return const_automaton_iterator (aid_map_.begin ());
  }
  
  const_automaton_iterator
  automaton_end ()
  {
    return const_automaton_iterator (aid_map_.end ());
  }

  void
  create_system_automaton (physical_address_t initrd_begin,
			   physical_address_t initrd_end)
  {
    initrd_begin_ = initrd_begin;
    initrd_end_ = initrd_end;

    // Create the automaton.
    // Automaton can execute privileged instructions.  (Needed when system allocator calls invlpg when mapping).
    // Automaton can manipulate virtual memory.  (Could be done with buffers but this way is more direct.)
    // Automaton can access kernel data.  (That is where the code/data reside.)
    system_automaton_ = create (true, vm::USER, vm::USER);

    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = system_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						 reinterpret_cast<logical_address_t> (&rodata_end));
    r = system_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
					       reinterpret_cast<logical_address_t> (&data_end));
    r = system_automaton_->insert_vm_area (area);
    kassert (r);

    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = system_automaton_->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
    r = system_automaton_->insert_stack_area (stack_area);
    kassert (r);
    
    // Add the actions.
    system_automaton_->add_action<system_automaton::first_traits> (&system_automaton::first);
    system_automaton_->add_action<system_automaton::init_traits> (&system_automaton::init);
    system_automaton_->add_action<system_automaton::bindsys_traits> (&system_automaton::bindsys);
    
    // Bootstrap.
    checked_schedule (system_automaton_, reinterpret_cast<const void*> (&system_automaton::first));
  }

  static void
  create_action_test_automaton ()
  {
    // Create automata to test actions.
    {    
      // Create the automaton.
      // Can't execute privileged instructions.
      // Can't manipulate virtual memory.
      // Can access kernel code/data.
      input_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
      
      // Create the memory map.
      vm_area_base* area;
      bool r;

      // Text.
      area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
						 reinterpret_cast<logical_address_t> (&text_end));
      r = input_automaton_->insert_vm_area (area);
      kassert (r);
      
      // Read-only data.
      area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						   reinterpret_cast<logical_address_t> (&rodata_end));
      r = input_automaton_->insert_vm_area (area);
      kassert (r);
      
      // Data.
      area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
      r = input_automaton_->insert_vm_area (area);
      kassert (r);
      
      // Heap.
      vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
      r = input_automaton_->insert_heap_area (heap_area);
      kassert (r);
      
      // Stack.
      vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
      r = input_automaton_->insert_stack_area (stack_area);
      kassert (r);
      
      // Fourth, add the actions.
      input_automaton_->add_action<action_test::np_nb_nc_input1_traits> (&action_test::np_nb_nc_input1);
      input_automaton_->add_action<action_test::np_nb_nc_input2_traits> (&action_test::np_nb_nc_input2);
      input_automaton_->add_action<action_test::np_nb_nc_input3_traits> (&action_test::np_nb_nc_input3);
      input_automaton_->add_action<action_test::np_nb_c_input1_traits> (&action_test::np_nb_c_input1);
      input_automaton_->add_action<action_test::np_nb_c_input2_traits> (&action_test::np_nb_c_input2);
      input_automaton_->add_action<action_test::np_nb_c_input3_traits> (&action_test::np_nb_c_input3);
      input_automaton_->add_action<action_test::np_b_nc_input1_traits> (&action_test::np_b_nc_input1);
      input_automaton_->add_action<action_test::np_b_nc_input2_traits> (&action_test::np_b_nc_input2);
      input_automaton_->add_action<action_test::np_b_nc_input3_traits> (&action_test::np_b_nc_input3);
      input_automaton_->add_action<action_test::np_b_c_input1_traits> (&action_test::np_b_c_input1);
      input_automaton_->add_action<action_test::np_b_c_input2_traits> (&action_test::np_b_c_input2);
      input_automaton_->add_action<action_test::np_b_c_input3_traits> (&action_test::np_b_c_input3);
      input_automaton_->add_action<action_test::p_nb_nc_input1_traits> (&action_test::p_nb_nc_input1);
      input_automaton_->add_action<action_test::p_nb_nc_input2_traits> (&action_test::p_nb_nc_input2);
      input_automaton_->add_action<action_test::p_nb_nc_input3_traits> (&action_test::p_nb_nc_input3);
      input_automaton_->add_action<action_test::p_nb_c_input1_traits> (&action_test::p_nb_c_input1);
      input_automaton_->add_action<action_test::p_nb_c_input2_traits> (&action_test::p_nb_c_input2);
      input_automaton_->add_action<action_test::p_nb_c_input3_traits> (&action_test::p_nb_c_input3);
      input_automaton_->add_action<action_test::p_b_nc_input1_traits> (&action_test::p_b_nc_input1);
      input_automaton_->add_action<action_test::p_b_nc_input2_traits> (&action_test::p_b_nc_input2);
      input_automaton_->add_action<action_test::p_b_nc_input3_traits> (&action_test::p_b_nc_input3);
      input_automaton_->add_action<action_test::p_b_c_input1_traits> (&action_test::p_b_c_input1);
      input_automaton_->add_action<action_test::p_b_c_input2_traits> (&action_test::p_b_c_input2);
      input_automaton_->add_action<action_test::p_b_c_input3_traits> (&action_test::p_b_c_input3);
      input_automaton_->add_action<action_test::ap_nb_nc_input1_traits> (&action_test::ap_nb_nc_input1);
      input_automaton_->add_action<action_test::ap_nb_nc_input2_traits> (&action_test::ap_nb_nc_input2);
      input_automaton_->add_action<action_test::ap_nb_nc_input3_traits> (&action_test::ap_nb_nc_input3);
      input_automaton_->add_action<action_test::ap_nb_c_input1_traits> (&action_test::ap_nb_c_input1);
      input_automaton_->add_action<action_test::ap_nb_c_input2_traits> (&action_test::ap_nb_c_input2);
      input_automaton_->add_action<action_test::ap_nb_c_input3_traits> (&action_test::ap_nb_c_input3);
      input_automaton_->add_action<action_test::ap_b_nc_input1_traits> (&action_test::ap_b_nc_input1);
      input_automaton_->add_action<action_test::ap_b_nc_input2_traits> (&action_test::ap_b_nc_input2);
      input_automaton_->add_action<action_test::ap_b_nc_input3_traits> (&action_test::ap_b_nc_input3);
      input_automaton_->add_action<action_test::ap_b_c_input1_traits> (&action_test::ap_b_c_input1);
      input_automaton_->add_action<action_test::ap_b_c_input2_traits> (&action_test::ap_b_c_input2);
      input_automaton_->add_action<action_test::ap_b_c_input3_traits> (&action_test::ap_b_c_input3);
    }

    {
      // Create the automaton.
      // Can't execute privileged instructions.
      // Can't manipulate virtual memory.
      // Can access kernel code/data.
      output_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
      
      // Create the automaton's memory map.
      vm_area_base* area;
      bool r;

      // Text.
      area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
						 reinterpret_cast<logical_address_t> (&text_end));
      r = output_automaton_->insert_vm_area (area);
      kassert (r);
      
      // Read-only data.
      area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						   reinterpret_cast<logical_address_t> (&rodata_end));
      r = output_automaton_->insert_vm_area (area);
      kassert (r);
      
      // Data.
      area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
      r = output_automaton_->insert_vm_area (area);
      kassert (r);
      
      // Heap.
      vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
      r = output_automaton_->insert_heap_area (heap_area);
      kassert (r);
      
      // Stack.
      vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
      r = output_automaton_->insert_stack_area (stack_area);
      kassert (r);
      
      // Add the actions.
      output_automaton_->add_action<action_test::init_traits> (&action_test::init);
      output_automaton_->add_action<action_test::np_nb_nc_output_traits> (&action_test::np_nb_nc_output);
      output_automaton_->add_action<action_test::np_nb_c_output_traits> (&action_test::np_nb_c_output);
      output_automaton_->add_action<action_test::np_b_nc_output_traits> (&action_test::np_b_nc_output);
      output_automaton_->add_action<action_test::np_b_c_output_traits> (&action_test::np_b_c_output);
      output_automaton_->add_action<action_test::p_nb_nc_output_traits> (&action_test::p_nb_nc_output);
      output_automaton_->add_action<action_test::p_nb_c_output_traits> (&action_test::p_nb_c_output);
      output_automaton_->add_action<action_test::p_b_nc_output_traits> (&action_test::p_b_nc_output);
      output_automaton_->add_action<action_test::p_b_c_output_traits> (&action_test::p_b_c_output);
      output_automaton_->add_action<action_test::ap_nb_nc_output_traits> (&action_test::ap_nb_nc_output);
      output_automaton_->add_action<action_test::ap_nb_c_output_traits> (&action_test::ap_nb_c_output);
      output_automaton_->add_action<action_test::ap_b_nc_output_traits> (&action_test::ap_b_nc_output);
      output_automaton_->add_action<action_test::ap_b_c_output_traits> (&action_test::ap_b_c_output);
      
      output_automaton_->add_action<action_test::np_internal_traits> (&action_test::np_internal);
      output_automaton_->add_action<action_test::p_internal_traits> (&action_test::p_internal);

      // Bind.
      bind<system_automaton::init_traits,
      	action_test::init_traits> (system_automaton_, &system_automaton::init, output_automaton_,
      				   output_automaton_, &action_test::init, system_automaton_);
      init_queue_->push_back (output_automaton_);

      bindsys_ = true;
    }
  }

  static void
  bind_action_test_automaton ()
  {
    if (output_automaton_ != 0 && input_automaton_ != 0) {
      bind<action_test::np_nb_nc_output_traits,
      	action_test::np_nb_nc_input1_traits> (output_automaton_, &action_test::np_nb_nc_output,
      					      input_automaton_, &action_test::np_nb_nc_input1, system_automaton_);

      bind<action_test::p_nb_nc_output_traits,
	action_test::np_nb_nc_input2_traits> (output_automaton_, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
					      input_automaton_, &action_test::np_nb_nc_input2, system_automaton_);

      action_test::ap_nb_nc_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_nb_nc_output_traits,
	action_test::np_nb_nc_input3_traits> (output_automaton_, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
					      input_automaton_, &action_test::np_nb_nc_input3, system_automaton_);

      bind<action_test::np_nb_c_output_traits,
	action_test::np_nb_c_input1_traits> (output_automaton_, &action_test::np_nb_c_output,
					     input_automaton_, &action_test::np_nb_c_input1, system_automaton_);

      bind<action_test::p_nb_c_output_traits,
	action_test::np_nb_c_input2_traits> (output_automaton_, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
					     input_automaton_, &action_test::np_nb_c_input2, system_automaton_);

      action_test::ap_nb_c_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_nb_c_output_traits,
	action_test::np_nb_c_input3_traits> (output_automaton_, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
					     input_automaton_, &action_test::np_nb_c_input3, system_automaton_);

      bind<action_test::np_b_nc_output_traits,
	action_test::np_b_nc_input1_traits> (output_automaton_, &action_test::np_b_nc_output,
					     input_automaton_, &action_test::np_b_nc_input1, system_automaton_);
      
      bind<action_test::p_b_nc_output_traits,
	action_test::np_b_nc_input2_traits> (output_automaton_, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
					     input_automaton_, &action_test::np_b_nc_input2, system_automaton_);
      
      action_test::ap_b_nc_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_b_nc_output_traits,
	action_test::np_b_nc_input3_traits> (output_automaton_, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
					     input_automaton_, &action_test::np_b_nc_input3, system_automaton_);
      
      bind<action_test::np_b_c_output_traits,
	action_test::np_b_c_input1_traits> (output_automaton_, &action_test::np_b_c_output,
					    input_automaton_, &action_test::np_b_c_input1, system_automaton_);
      
      bind<action_test::p_b_c_output_traits,
	action_test::np_b_c_input2_traits> (output_automaton_, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
					    input_automaton_, &action_test::np_b_c_input2, system_automaton_);
      
      action_test::ap_b_c_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_b_c_output_traits,
	action_test::np_b_c_input3_traits> (output_automaton_, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
					    input_automaton_, &action_test::np_b_c_input3, system_automaton_);
      
      bind<action_test::np_nb_nc_output_traits,
	action_test::p_nb_nc_input1_traits> (output_automaton_, &action_test::np_nb_nc_output,
					     input_automaton_, &action_test::p_nb_nc_input1, action_test::p_nb_nc_input1_parameter, system_automaton_);
      
      bind<action_test::p_nb_nc_output_traits,
	action_test::p_nb_nc_input2_traits> (output_automaton_, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
					     input_automaton_, &action_test::p_nb_nc_input2, action_test::p_nb_nc_input2_parameter, system_automaton_);
      
      action_test::ap_nb_nc_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_nb_nc_output_traits,
	action_test::p_nb_nc_input3_traits> (output_automaton_, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
					     input_automaton_, &action_test::p_nb_nc_input3, action_test::p_nb_nc_input3_parameter, system_automaton_);
      
      bind<action_test::np_nb_c_output_traits,
	action_test::p_nb_c_input1_traits> (output_automaton_, &action_test::np_nb_c_output,
					    input_automaton_, &action_test::p_nb_c_input1, action_test::p_nb_c_input1_parameter, system_automaton_);
      
      bind<action_test::p_nb_c_output_traits,
	action_test::p_nb_c_input2_traits> (output_automaton_, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
					    input_automaton_, &action_test::p_nb_c_input2, action_test::p_nb_c_input2_parameter, system_automaton_);
      
      action_test::ap_nb_c_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_nb_c_output_traits,
	action_test::p_nb_c_input3_traits> (output_automaton_, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
					    input_automaton_, &action_test::p_nb_c_input3, action_test::p_nb_c_input3_parameter, system_automaton_);
      
      bind<action_test::np_b_nc_output_traits,
	action_test::p_b_nc_input1_traits> (output_automaton_, &action_test::np_b_nc_output,
					    input_automaton_, &action_test::p_b_nc_input1, action_test::p_b_nc_input1_parameter, system_automaton_);
      
      bind<action_test::p_b_nc_output_traits,
	action_test::p_b_nc_input2_traits> (output_automaton_, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
					    input_automaton_, &action_test::p_b_nc_input2, action_test::p_b_nc_input2_parameter, system_automaton_);
      
      action_test::ap_b_nc_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_b_nc_output_traits,
	action_test::p_b_nc_input3_traits> (output_automaton_, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
					    input_automaton_, &action_test::p_b_nc_input3, action_test::p_b_nc_input3_parameter, system_automaton_);
      
      bind<action_test::np_b_c_output_traits,
	action_test::p_b_c_input1_traits> (output_automaton_, &action_test::np_b_c_output,
					   input_automaton_, &action_test::p_b_c_input1, action_test::p_b_c_input1_parameter, system_automaton_);
      
      bind<action_test::p_b_c_output_traits,
	action_test::p_b_c_input2_traits> (output_automaton_, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
					   input_automaton_, &action_test::p_b_c_input2, action_test::p_b_c_input2_parameter, system_automaton_);
      
      action_test::ap_b_c_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_b_c_output_traits,
	action_test::p_b_c_input3_traits> (output_automaton_, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
					   input_automaton_, &action_test::p_b_c_input3, action_test::p_b_c_input3_parameter, system_automaton_);
      
      action_test::ap_nb_nc_input1_parameter = output_automaton_->aid ();
      bind<action_test::np_nb_nc_output_traits,
	action_test::ap_nb_nc_input1_traits> (output_automaton_, &action_test::np_nb_nc_output,
					      input_automaton_, &action_test::ap_nb_nc_input1, action_test::ap_nb_nc_input1_parameter, system_automaton_);
      
      action_test::ap_nb_nc_input2_parameter = output_automaton_->aid ();
      bind<action_test::p_nb_nc_output_traits,
	action_test::ap_nb_nc_input2_traits> (output_automaton_, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
					      input_automaton_, &action_test::ap_nb_nc_input2, action_test::ap_nb_nc_input2_parameter, system_automaton_);
      
      action_test::ap_nb_nc_input3_parameter = output_automaton_->aid ();
      action_test::ap_nb_nc_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_nb_nc_output_traits,
	action_test::ap_nb_nc_input3_traits> (output_automaton_, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
					      input_automaton_, &action_test::ap_nb_nc_input3, action_test::ap_nb_nc_input3_parameter, system_automaton_);
      
      action_test::ap_nb_c_input1_parameter = output_automaton_->aid ();
      bind<action_test::np_nb_c_output_traits,
	action_test::ap_nb_c_input1_traits> (output_automaton_, &action_test::np_nb_c_output,
					     input_automaton_, &action_test::ap_nb_c_input1, action_test::ap_nb_c_input1_parameter, system_automaton_);
      
      action_test::ap_nb_c_input2_parameter = output_automaton_->aid ();
      bind<action_test::p_nb_c_output_traits,
	action_test::ap_nb_c_input2_traits> (output_automaton_, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
					     input_automaton_, &action_test::ap_nb_c_input2, action_test::ap_nb_c_input2_parameter, system_automaton_);
      
      action_test::ap_nb_c_input3_parameter = output_automaton_->aid ();
      action_test::ap_nb_c_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_nb_c_output_traits,
	action_test::ap_nb_c_input3_traits> (output_automaton_, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
					     input_automaton_, &action_test::ap_nb_c_input3, action_test::ap_nb_c_input3_parameter, system_automaton_);
      
      action_test::ap_b_nc_input1_parameter = output_automaton_->aid ();
      bind<action_test::np_b_nc_output_traits,
	action_test::ap_b_nc_input1_traits> (output_automaton_, &action_test::np_b_nc_output,
					     input_automaton_, &action_test::ap_b_nc_input1, action_test::ap_b_nc_input1_parameter, system_automaton_);
      
      action_test::ap_b_nc_input2_parameter = output_automaton_->aid ();
      bind<action_test::p_b_nc_output_traits,
	action_test::ap_b_nc_input2_traits> (output_automaton_, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
					     input_automaton_, &action_test::ap_b_nc_input2, action_test::ap_b_nc_input2_parameter, system_automaton_);
      
      action_test::ap_b_nc_input3_parameter = output_automaton_->aid ();
      action_test::ap_b_nc_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_b_nc_output_traits,
	action_test::ap_b_nc_input3_traits> (output_automaton_, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
					     input_automaton_, &action_test::ap_b_nc_input3, action_test::ap_b_nc_input3_parameter, system_automaton_);
    
      action_test::ap_b_c_input1_parameter = output_automaton_->aid ();
      bind<action_test::np_b_c_output_traits,
	action_test::ap_b_c_input1_traits> (output_automaton_, &action_test::np_b_c_output,
					    input_automaton_, &action_test::ap_b_c_input1, action_test::ap_b_c_input1_parameter, system_automaton_);
    
      action_test::ap_b_c_input2_parameter = output_automaton_->aid ();
      bind<action_test::p_b_c_output_traits,
	action_test::ap_b_c_input2_traits> (output_automaton_, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
					    input_automaton_, &action_test::ap_b_c_input2, action_test::ap_b_c_input2_parameter, system_automaton_);
    
      action_test::ap_b_c_input3_parameter = output_automaton_->aid ();
      action_test::ap_b_c_output_parameter = input_automaton_->aid ();
      bind<action_test::ap_b_c_output_traits,
	action_test::ap_b_c_input3_traits> (output_automaton_, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
					    input_automaton_, &action_test::ap_b_c_input3, action_test::ap_b_c_input3_parameter, system_automaton_);

      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_nb_nc_output));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_nb_nc_output), aid_cast (action_test::p_nb_nc_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_nb_nc_output), aid_cast (action_test::ap_nb_nc_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_nb_c_output));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_nb_c_output), aid_cast (action_test::p_nb_c_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_nb_c_output), aid_cast (action_test::ap_nb_c_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_b_nc_output));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_b_nc_output), aid_cast (action_test::p_b_nc_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_b_nc_output), aid_cast (action_test::ap_b_nc_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_b_c_output));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_b_c_output), aid_cast (action_test::p_b_c_output_parameter));
      checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_b_c_output), aid_cast (action_test::ap_b_c_output_parameter));
    }
  }

  static void
  create_buffer_test_automaton ()
  {
    // Create the automaton.
    // Can't execute privileged instructions.
    // Can't manipulate virtual memory.
    // Can access kernel code/data.
    buffer_test_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = buffer_test_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						 reinterpret_cast<logical_address_t> (&rodata_end));
    r = buffer_test_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
    r = buffer_test_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = buffer_test_automaton_->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
    r = buffer_test_automaton_->insert_stack_area (stack_area);
    kassert (r);
      
    // Add the actions.
    buffer_test_automaton_->add_action<buffer_test::init_traits> (&buffer_test::init);

    // Bind.
    bind<system_automaton::init_traits,
      buffer_test::init_traits> (system_automaton_, &system_automaton::init, buffer_test_automaton_,
				 buffer_test_automaton_, &buffer_test::init, system_automaton_);
    init_queue_->push_back (buffer_test_automaton_);
    bindsys_ = true;
  }

  static void
  bind_buffer_test_automaton ()
  {
    if (buffer_test_automaton_ != 0) {

    }
  }

  static void
  create_ramdisk_automaton ()
  {
    // Create the automaton.
    // Can't execute privileged instructions.
    // Can't manipulate virtual memory.
    // Can access kernel code/data.
    ramdisk_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = ramdisk_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
						 reinterpret_cast<logical_address_t> (&rodata_end));
    r = ramdisk_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
						 reinterpret_cast<logical_address_t> (&data_end));
    r = ramdisk_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = ramdisk_automaton_->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
    r = ramdisk_automaton_->insert_stack_area (stack_area);
    kassert (r);

    // Add the actions.
    ramdisk_automaton_->add_action<ramdisk::init_traits> (&ramdisk::init);
    ramdisk_automaton_->add_action<ramdisk::info_request_traits> (&ramdisk::info_request);
    ramdisk_automaton_->add_action<ramdisk::info_response_traits> (&ramdisk::info_response);
    ramdisk_automaton_->add_action<ramdisk::read_request_traits> (&ramdisk::read_request);
    ramdisk_automaton_->add_action<ramdisk::read_response_traits> (&ramdisk::read_response);
    ramdisk_automaton_->add_action<ramdisk::write_request_traits> (&ramdisk::write_request);
    ramdisk_automaton_->add_action<ramdisk::write_response_traits> (&ramdisk::write_response);
    
    // Bind.
    bind<system_automaton::init_traits,
      ramdisk::init_traits> (system_automaton_, &system_automaton::init, ramdisk_automaton_,
			     ramdisk_automaton_, &ramdisk::init, system_automaton_);
    init_queue_->push_back (ramdisk_automaton_);
    
    buffer* b = new (system_alloc ()) buffer (physical_address_to_frame (initrd_begin_), physical_address_to_frame (align_up (initrd_end_, PAGE_SIZE)));
    ramdisk::bid = ramdisk_automaton_->buffer_create (*b);
    destroy (b, system_alloc ());
    bindsys_ = true;
  }

  static void
  bind_ramdisk_automaton ()
  {
    if (ramdisk_automaton_ != 0) {

    }
  }

  static void
  create_ext2_automaton ()
  {
    // Create the automaton.
    // Can't execute privileged instructions.
    // Can't manipulate virtual memory.
    // Can access kernel code/data.
    ext2_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
    					       reinterpret_cast<logical_address_t> (&text_end));
    r = ext2_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Read-only data.
    area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
    						 reinterpret_cast<logical_address_t> (&rodata_end));
    r = ext2_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
    						 reinterpret_cast<logical_address_t> (&data_end));
    r = ext2_automaton_->insert_vm_area (area);
    kassert (r);
    
    // Heap.
    vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = ext2_automaton_->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
    r = ext2_automaton_->insert_stack_area (stack_area);
    kassert (r);

    // Add the actions.
    ext2_automaton_->add_action<ext2::init_traits> (&ext2::init);
    ext2_automaton_->add_action<ext2::info_request_traits> (&ext2::info_request);
    ext2_automaton_->add_action<ext2::info_response_traits> (&ext2::info_response);
    ext2_automaton_->add_action<ext2::read_request_traits> (&ext2::read_request);
    ext2_automaton_->add_action<ext2::read_response_traits> (&ext2::read_response);
    ext2_automaton_->add_action<ext2::write_request_traits> (&ext2::write_request);
    ext2_automaton_->add_action<ext2::write_response_traits> (&ext2::write_response);
    ext2_automaton_->add_action<ext2::generate_read_request_traits> (&ext2::generate_read_request);
    ext2_automaton_->add_action<ext2::generate_write_request_traits> (&ext2::generate_write_request);

    // Bind.
    bind<system_automaton::init_traits,
      ext2::init_traits> (system_automaton_, &system_automaton::init, ext2_automaton_,
    			  ext2_automaton_, &ext2::init, system_automaton_);
    init_queue_->push_back (ext2_automaton_);
    bindsys_ = true;
  }

  static void
  bind_ext2_automaton ()
  {
    if (ext2_automaton_ != 0) {
      bind<ext2::info_request_traits,
        ramdisk::info_request_traits> (ext2_automaton_, &ext2::info_request,
				       ramdisk_automaton_, &ramdisk::info_request, ext2_automaton_->aid (), system_automaton_);
      checked_schedule (ext2_automaton_, reinterpret_cast<const void*> (&ext2::info_request));
      
      bind<ramdisk::info_response_traits,
        ext2::info_response_traits> (ramdisk_automaton_, &ramdisk::info_response, ext2_automaton_->aid (),
				     ext2_automaton_, &ext2::info_response, system_automaton_);
      checked_schedule (ramdisk_automaton_, reinterpret_cast<const void*> (&ramdisk::info_response), ext2_automaton_->aid ());

      bind<ext2::read_request_traits,
        ramdisk::read_request_traits> (ext2_automaton_, &ext2::read_request,
				       ramdisk_automaton_, &ramdisk::read_request, ext2_automaton_->aid (), system_automaton_);
      checked_schedule (ext2_automaton_, reinterpret_cast<const void*> (&ext2::read_request));
      
      bind<ramdisk::read_response_traits,
        ext2::read_response_traits> (ramdisk_automaton_, &ramdisk::read_response, ext2_automaton_->aid (),
				     ext2_automaton_, &ext2::read_response, system_automaton_);
      checked_schedule (ramdisk_automaton_, reinterpret_cast<const void*> (&ramdisk::read_response), ext2_automaton_->aid ());

      bind<ext2::write_request_traits,
        ramdisk::write_request_traits> (ext2_automaton_, &ext2::write_request,
				       ramdisk_automaton_, &ramdisk::write_request, ext2_automaton_->aid (), system_automaton_);
      checked_schedule (ext2_automaton_, reinterpret_cast<const void*> (&ext2::write_request));
      
      bind<ramdisk::write_response_traits,
        ext2::write_response_traits> (ramdisk_automaton_, &ramdisk::write_response, ext2_automaton_->aid (),
				     ext2_automaton_, &ext2::write_response, system_automaton_);
      checked_schedule (ramdisk_automaton_, reinterpret_cast<const void*> (&ramdisk::write_response), ext2_automaton_->aid ());
    }
  }

  static bool
  bindsys_precondition ()
  {
    return bindsys_ && init_queue_->empty ();
  }

  static void
  schedule ()
  {
    if (!init_queue_->empty ()) {
      scheduler_->add<init_traits> (&init, init_queue_->front ());
    }
    if (bindsys_precondition ()) {
      scheduler_->add<bindsys_traits> (&bindsys);
    }
  }

  void
  first (void)
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();
    init_queue_ = new (alloc_type ()) init_queue_type ();

    //create_action_test_automaton ();
    //create_buffer_test_automaton ();
    create_ramdisk_automaton ();
    create_ext2_automaton ();

    schedule ();
    scheduler_->finish<first_traits> ();
  }

  void
  init (automaton* p)
  {
    scheduler_->remove<init_traits> (&init, p);
    kassert (p == init_queue_->front ());
    init_queue_->pop_front ();
    schedule ();
    scheduler_->finish<init_traits> (true);
  }

  void
  bindsys (void)
  {
    if (bindsys_precondition ()) {
      bind_action_test_automaton ();
      bind_buffer_test_automaton ();
      bind_ramdisk_automaton ();
      bind_ext2_automaton ();
      bindsys_ = false;
      schedule ();
      scheduler_->finish<bindsys_traits> ();
    }
    else {
      schedule ();
      scheduler_->finish<bindsys_traits> ();
    }
  }
}
