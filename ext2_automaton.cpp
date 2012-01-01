#include "ext2_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kout.hpp"
#include "kassert.hpp"
#include <string.h>

namespace ext2 {

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
  
  enum rr_status_t {
    NOT_SENT,
    SENT,
    RECEIVED,
  };

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static rr_status_t info_status_ = NOT_SENT;
  static ramdisk::info_response_t info_;

  typedef std::deque<ramdisk::block_num_t, allocator_type<ramdisk::block_num_t> > read_queue_type;
  static read_queue_type* read_queue_ = 0;

  typedef std::deque<ramdisk::block_num_t, allocator_type<ramdisk::block_num_t> > write_queue_type;
  static write_queue_type* write_queue_ = 0;

  static bool
  info_request_precondition ()
  {
    return info_status_ == NOT_SENT &&
      syscall::binding_count (&info_request) != 0 &&
      syscall::binding_count (&info_response) != 0;
  }

  static bool
  read_request_precondition ()
  {
    return !read_queue_->empty () &&
      syscall::binding_count (&read_request) != 0 &&
      syscall::binding_count (&read_response) != 0;
  }

  static bool
  write_request_precondition ()
  {
    return !write_queue_->empty () &&
      syscall::binding_count (&write_request) != 0 &&
      syscall::binding_count (&write_response) != 0;
  }

  static bool
  generate_read_request_precondition ()
  {
    return false; //return info_status_ == RECEIVED && read_queue_->empty ();
  }

  static bool
  generate_write_request_precondition ()
  {
    return false; //return info_status_ == RECEIVED && write_queue_->empty ();
  }

  static void
  schedule ()
  {
    if (info_request_precondition ()) {
      scheduler_->add<info_request_traits> (&info_request);
    }
    if (read_request_precondition ()) {
      scheduler_->add<read_request_traits> (&read_request);
    }
    if (write_request_precondition ()) {
      scheduler_->add<write_request_traits> (&write_request);
    }
    if (generate_read_request_precondition ()) {
      scheduler_->add<generate_read_request_traits> (&generate_read_request);
    }
    if (generate_write_request_precondition ()) {
      scheduler_->add<generate_write_request_traits> (&generate_write_request);
    }
  }

  // Init.
  void
  init (void)
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();

    read_queue_ = new (alloc_type ()) read_queue_type ();
    write_queue_ = new (alloc_type ()) write_queue_type ();

    schedule ();
    scheduler_->finish<init_traits> ();
  }

  void
  info_request ()
  {
    scheduler_->remove<info_request_traits> (&info_request);
    if (info_request_precondition ()) {
      info_status_ = SENT;
      schedule ();
      scheduler_->finish<info_request_traits> (true);
    }
    else {
      schedule ();
      scheduler_->finish<info_request_traits> ();
    }
  }

  void
  info_response (ramdisk::info_response_t info)
  {
    info_ = info;
    info_status_ = RECEIVED;
    schedule ();
    scheduler_->finish<info_response_traits> ();
  }

  void
  read_request ()
  {
    scheduler_->remove<read_request_traits> (&read_request);
    if (read_request_precondition ()) {
      ramdisk::read_request_t request (read_queue_->front ());
      read_queue_->pop_front ();
      schedule ();
      scheduler_->finish<read_request_traits> (&request);
    }
    else {
      schedule ();
      scheduler_->finish<read_request_traits> ();
    }
  }

  void
  read_response (bid_t bid,
		 ramdisk::read_response_t response)
  {
    if (response.error == ramdisk::SUCCESS) {
      const char* c = static_cast<const char*> (syscall::buffer_map (bid));
      kassert (c != 0);
      kout << c << endl;
    }
    schedule ();
    scheduler_->finish<read_response_traits> ();
  }

  void
  write_request ()
  {
    scheduler_->remove<write_request_traits> (&write_request);
    if (write_request_precondition ()) {
      ramdisk::write_request_t request (write_queue_->front ());
      bid_t b = syscall::buffer_create (syscall::getpagesize ());
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
      strcpy (c, "HA!!  I wrote the first block");
      write_queue_->pop_front ();
      schedule ();
      scheduler_->finish<write_request_traits> (b, &request);
    }
    else {
      schedule ();
      scheduler_->finish<write_request_traits> ();
    }
  }
  
  void
  write_response (ramdisk::write_response_t response)
  {
    kassert (response.error == ramdisk::SUCCESS);
    schedule ();
    scheduler_->finish<write_response_traits> ();
  }

  void
  generate_read_request ()
  {
    scheduler_->remove<generate_read_request_traits> (&generate_read_request);
    if (generate_read_request_precondition ()) {
      // Request the first block.
      read_queue_->push_back (0);
    }
    schedule ();
    scheduler_->finish<generate_read_request_traits> ();
  }

  void
  generate_write_request ()
  {
    scheduler_->remove<generate_write_request_traits> (&generate_write_request);
    if (generate_write_request_precondition ()) {
      // Write the first block.
      write_queue_->push_back (0);
    }
    schedule ();
    scheduler_->finish<generate_write_request_traits> ();
  }

}
