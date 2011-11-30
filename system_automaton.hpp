#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

/*
  File
  ----
  system_automaton.hpp
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

class system_automaton {
public:
  void
  schedule_action (void* /* action_entry_point*/,
		   parameter_t /* parameter*/)
  {
    kassert (0);
  }

  void
  finish_action (bool /*output_status*/,
		 value_t /*output_value*/)
  {
    kassert (0);
  }

  logical_address
  alloc (size_t /*size*/)
  {
    kassert (0);
    return logical_address ();
  }

  void
  unknown_system_call (void)
  {
    kassert (0);
  }
};

// #include <memory>
// #include "automaton_interface.hpp"
// #include "boot_automaton.hpp"
// #include "automaton.hpp"
// #include "fifo_scheduler.hpp"

// /* Size of the stack used for "system" automata.
//    Must be large enough for functions and interrupts. */
// #define SYSTEM_STACK_SIZE 0x1000

// /* Size of the stack to use when switching between automata. */
// #define SWITCH_STACK_SIZE 256

// /* Locations to use for data segments of system automata. */
// const logical_address SYSTEM_DATA_BEGIN (reinterpret_cast<void*> (0X00100000));
// const logical_address SYSTEM_DATA_END (reinterpret_cast<void*> (0X00101000));

// /* Markers for the kernel. */
// extern int text_begin;
// extern int text_end;
// extern int rodata_begin;
// extern int rodata_end;
// extern int data_begin;
// extern int data_end;

// // Symbols for the actions.
// extern int sa_first;
// extern int sa_init;
// extern int sa_create_request;
// extern int sa_create_response;
// extern int sa_bind_request;
// extern int sa_bind_response;
// extern int sa_unbind_request;
// extern int sa_unbind_response;
// extern int sa_destroy_request;
// extern int sa_destroy_response;

// extern int initrd_init;

// template <class AllocatorTag, template <typename> class Allocator>
// class system_automaton {
// private:
//   typedef automaton<AllocatorTag, Allocator> automaton_type;
//   static automaton_interface* instance_;
//   static fifo_scheduler<AllocatorTag, Allocator>* scheduler_;
//   static bool flag_;

// public:
//   /* Does not return. */
//   static void
//   initialize (logical_address placement_begin,
// 	      logical_address placement_end)
//   {
//     kassert (instance_ == 0);
    
//     /* To create the data structures for the system automaton and scheduler, we need dynamic memory allocation.
//        All memory is allocated through a system call.
//        System calls require that an automaton be executing.
//        For an automaton to be executing, we need the data structures for the system automaton and scheduler.
       
//        Uh oh.
       
//        To break the recursion, we'll manually create an automaton that behaves correctly so that dynamic memory can be initialized.
//     */
    
//     boot_automaton<AllocatorTag> boot_automaton (placement_begin, placement_end);
    
//     /* Use it as the system automaton. */
//     instance_ = &boot_automaton;
    
//     /* Initialize the scheduler so it thinks the system automaton is executing. */
//     scheduler<AllocatorTag, Allocator>::initialize (&boot_automaton);
    
//     /* Now, we can start allocating. */
    
//     /* Allocate and set the switch stack for the scheduler. */
//     void* switch_stack = new (AllocatorTag ()) char[SWITCH_STACK_SIZE];
//     kassert (switch_stack != 0);
//     scheduler<AllocatorTag, Allocator>::set_switch_stack (logical_address (switch_stack), SWITCH_STACK_SIZE);
    
//     /* Allocate the real system automaton.
//        The system automaton's ceiling is the start of the paging data structures.
//        This is also used as the stack pointer. */
//     automaton<AllocatorTag, Allocator>* sys_automaton = new (AllocatorTag ()) automaton<AllocatorTag, Allocator> (RING0, vm_manager_page_directory_physical_address (), vm_manager_page_directory_logical_address (), vm_manager_page_directory_logical_address (), SUPERVISOR);
    
//     {
//       // Create a memory map for the system automatom.
//       // Reserve low memory.
//       kassert (sys_automaton->insert_vm_area (vm_reserved_area<AllocatorTag> (logical_address (0), logical_address (reinterpret_cast<void*> (0x100000)))));
//       // Program text.
//       kassert (sys_automaton->insert_vm_area (vm_text_area<AllocatorTag> (logical_address (&text_begin), logical_address (&text_end), SUPERVISOR)));
//       // Program read-only data.
//       kassert (sys_automaton->insert_vm_area (vm_rodata_area<AllocatorTag> (logical_address (&rodata_begin), logical_address (&rodata_end), SUPERVISOR)));
//       // Program data.
//       kassert (sys_automaton->insert_vm_area (vm_data_area<AllocatorTag> (logical_address (&data_begin), logical_address (&data_end), SUPERVISOR)));
//       // Data that has been allocated.
//       kassert (sys_automaton->insert_vm_area (boot_automaton.get_data_area ()));
//     }
//     {
//       /* Add a stack. */
//       vm_stack_area<AllocatorTag> stack (vm_manager_page_directory_logical_address () - SYSTEM_STACK_SIZE,
// 					 vm_manager_page_directory_logical_address (),
// 					 SUPERVISOR);
//       kassert (sys_automaton->insert_vm_area (stack));
//       /* When call finish_action below, we will switch to the new stack.
// 	 If we don't back the stack with physical pages, the first stack operation will triple fault.
// 	 The scenario that we must avoid is:
// 	 1.  Stack operation (recall that exceptions/interrupts use the stack).
// 	 2.  No stack so page fault (exception).
// 	 3.  Page fault will try to use the stack, same result.
// 	 4.  Triple fault.  Game over.
	 
// 	 So, we need to back the stack with physical pages.
//       */
//       logical_address ptr;
//       for (ptr = stack.begin (); ptr < stack.end (); ptr += PAGE_SIZE) {
// 	vm_manager_map (ptr, frame_manager::alloc (), SUPERVISOR, WRITABLE);
//       }
//     }
    
//     /* Add the actions. */
//     sys_automaton->add_action (&sa_first, INTERNAL);
//     sys_automaton->add_action (&sa_init, OUTPUT);
//     sys_automaton->add_action (&sa_create_request, INPUT);
//     sys_automaton->add_action (&sa_create_response, OUTPUT);
//     sys_automaton->add_action (&sa_bind_request, INPUT);
//     sys_automaton->add_action (&sa_bind_response, OUTPUT);
//     sys_automaton->add_action (&sa_unbind_request, INPUT);
//     sys_automaton->add_action (&sa_unbind_response, OUTPUT);
//     sys_automaton->add_action (&sa_destroy_request, INPUT);
//     sys_automaton->add_action (&sa_destroy_response, OUTPUT);
    
//     /* Set the system automaton. */
//     instance_ = sys_automaton;
    
//     /* Create a scheduler. */
//     scheduler_ = new (AllocatorTag ()) fifo_scheduler<AllocatorTag, Allocator> ();
//     kassert (scheduler_ != 0);
    
//     binding_manager<AllocatorTag, Allocator>::initialize ();
    
//     /* Go! */
//     scheduler<AllocatorTag, Allocator>::schedule_action (instance_, &sa_first, 0);
    
//     /* Start the scheduler.  Doesn't return. */
//     scheduler<AllocatorTag, Allocator>::finish_action (0, 0);
    
//     kassert (0);
//   }

//   static automaton_interface*
//   get_instance (void)
//   {
//     kassert (instance_ != 0);
//     return instance_;
//   }
  
//   static bool
//   sa_first_precondition ()
//   {
//     return flag_;
//   }

//   static void
//   sa_first_effect ()
//   {
//     /* Create the initrd automaton. */

//     /* First, create a new page directory. */

//     /* Reserve some logical address space for the page directory. */
//     page_directory_t* page_directory = static_cast<page_directory_t*> (instance_->reserve (PAGE_SIZE).value ());
//     kassert (page_directory != 0);
//     /* Allocate a frame. */
//     frame frame = frame_manager::alloc ();
//     /* Map the page directory. */
//     vm_manager_map (logical_address (page_directory), frame, SUPERVISOR, WRITABLE);
//     /* Initialize the page directory with a copy of the current page directory. */
//     page_directory_initialize_with_current (page_directory, frame);
//     /* Unmap. */
//     vm_manager_unmap (logical_address (page_directory));
//     /* Unreserve. */
//     instance_->unreserve (logical_address (page_directory));

//     /* Switch to the new page directory. */
//     vm_manager_switch_to_directory (physical_address (frame));
  
//     /* Second, create the automaton. */
//     automaton_type* initrd = new (AllocatorTag ()) automaton_type (RING0, physical_address (frame), KERNEL_VIRTUAL_BASE, KERNEL_VIRTUAL_BASE, SUPERVISOR);

//     /* Third, create the automaton's memory map. */
//     {
//       /* Add a data area. */
//       kassert (initrd->insert_vm_area (vm_data_area<AllocatorTag> (SYSTEM_DATA_BEGIN, SYSTEM_DATA_END, SUPERVISOR)));

//       /* Add a stack area. */
//       vm_stack_area<AllocatorTag> stack (initrd->get_stack_pointer () - SYSTEM_STACK_SIZE,
// 					 initrd->get_stack_pointer (),
// 					 SUPERVISOR);
//       kassert (initrd->insert_vm_area (stack));

//       /* Back with physical pages.  See comment in initialize (). */
//       logical_address ptr;
//       for (ptr = stack.begin (); ptr < stack.end (); ptr += PAGE_SIZE) {
// 	vm_manager_map (ptr, frame_manager::alloc (), SUPERVISOR, WRITABLE);
//       }
//     }

//     /* Fourth, add the actions. */
//     initrd->add_action (&initrd_init, INPUT);

//     /* Fifth, bind. */
//     binding_manager<AllocatorTag, Allocator>::bind (instance_, &sa_init, reinterpret_cast<parameter_t> (initrd),
// 						    initrd, &initrd_init, 0);
  
//     /* Sixth, initialize the new automaton. */
//     scheduler_->add (&sa_init, (parameter_t)initrd);

//     /* Create the VFS automaton. */

//     /* Create the init automaton. */

//     kputs (__func__); kputs ("\n");

//     flag_ = false;
//   }

//   static void
//   sa_first_schedule () {
//     schedule ();
//   }

//   static bool
//   init_precondition (automaton_type*)
//   {
//     return true;
//   }

//   static void
//   init_effect (automaton_type* automaton)
//   {
//     kputs (__func__); kputs (" "); kputp (automaton); kputs ("\n");
//   }

//   static void
//   init_schedule (automaton_type*) {
//     schedule ();
//   }

//   static void
//   create_request_effect () {
//     kassert (0);
//   }

//   static void
//   create_request_schedule () {
//     kassert (0);
//   }

//   static bool
//   create_response_precondition () {
//     kassert (0);
//     return false;
//   }

//   static void
//   create_response_effect () {
//     kassert (0);
//   }

//   static void
//   create_response_schedule () {
//     kassert (0);
//   }

//   static void
//   bind_request_effect () {
//     kassert (0);
//   }

//   static void
//   bind_request_schedule () {
//     kassert (0);
//   }

//   static bool
//   bind_response_precondition () {
//     kassert (0);
//     return false;
//   }

//   static void
//   bind_response_effect () {
//     kassert (0);
//   }

//   static void
//   bind_response_schedule () {
//     kassert (0);
//   }

//   static void
//   unbind_request_effect () {
//     kassert (0);
//   }

//   static void
//   unbind_request_schedule () {
//     kassert (0);
//   }

//   static bool
//   unbind_response_precondition () {
//     kassert (0);
//     return false;
//   }

//   static void
//   unbind_response_effect () {
//     kassert (0);
//   }

//   static void
//   unbind_response_schedule () {
//     kassert (0);
//   }

//   static void
//   destroy_request_effect () {
//     kassert (0);
//   }

//   static void
//   destroy_request_schedule () {
//     kassert (0);
//   }

//   static bool
//   destroy_response_precondition () {
//     kassert (0);
//     return false;
//   }

//   static void
//   destroy_response_effect () {
//     kassert (0);
//   }

//   static void
//   destroy_response_schedule () {
//     kassert (0);
//   }

//   static void
//   schedule ()
//   {
//     if (sa_first_precondition ()) {
//       scheduler_->add (&sa_first, 0);
//     }
//   }

//   static void
//   schedule_remove (void* action_entry_point,
// 		   parameter_t parameter)
//   {
//     kassert (0);
//   }

//   static void
//   schedule_finish (bool output_status,
// 		   value_t output_value)
//   {
//     kassert (0);
//   }

  
// };

#endif /* __system_automaton_hpp__ */

