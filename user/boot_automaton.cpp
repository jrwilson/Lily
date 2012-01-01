#include "boot_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"

namespace boot {

  static list_alloc_state state_;

  struct ext2_automaton_syscall : public list_alloc_syscall {
    static list_alloc_state&
    state ()
    {
      return state_;
    }
  };
  
  typedef list_alloc<ext2_automaton_syscall> alloc_type;
  
  template <typename T>
  struct allocator_type : public list_allocator<T, ext2_automaton_syscall> { };

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static void
  schedule ()
  {

  }

  // Init.
  void
  init (void)
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();
    
    schedule ();
    scheduler_->finish<init_traits> ();
  }

  void
  create_request (void)
  {
    schedule ();
    scheduler_->finish<init_traits> ();
  }

  void
  create_response (void)
  {
    schedule ();
    scheduler_->finish<init_traits> ();
  }
}
