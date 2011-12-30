#include "ramdisk_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kout.hpp"
#include "kassert.hpp"

namespace ramdisk {

  static list_alloc_state state_;
  
  struct ramdisk_automaton_syscall : public list_alloc_syscall {
    static list_alloc_state&
    state ()
    {
      return state_;
    }
  };
  
  typedef list_alloc<ramdisk_automaton_syscall> alloc_type;
  
  template <typename T>
  struct allocator_type : public list_allocator<T, ramdisk_automaton_syscall> { };

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  // The buffer to serve.
  bid_t bid;

  static size_t pagesize_;
  static info_response_t info_;
  
  // Queues for processing requests.
  typedef std::deque<aid_t, allocator_type<aid_t> > info_queue_type;
  static info_queue_type* info_queue_;

  typedef std::pair<aid_t, read_request_t> read_queue_item_type;
  typedef std::deque<read_queue_item_type, allocator_type<read_queue_item_type> > read_queue_type;
  static read_queue_type* read_queue_;

  typedef std::pair<aid_t, write_response_t> write_queue_item_type;
  typedef std::deque<write_queue_item_type, allocator_type<write_queue_item_type> > write_queue_type;
  static write_queue_type* write_queue_;

  static void
  schedule ()
  {
    if (!info_queue_->empty ()) {
      scheduler_->add<info_response_traits> (&info_response, info_queue_->front ());
    }
    if (!read_queue_->empty ()) {
      scheduler_->add<read_response_traits> (&read_response, read_queue_->front ().first);
    }
    if (!write_queue_->empty ()) {
      scheduler_->add<write_response_traits> (&write_response, write_queue_->front ().first);
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
    pagesize_ = syscall::getpagesize ();
    // Calculate the number of blocks.
    info_.block_count = syscall::buffer_size (bid) / pagesize_;
    // Allocate the queues.
    info_queue_ = new (alloc_type ()) info_queue_type ();
    read_queue_ = new (alloc_type ()) read_queue_type ();
    write_queue_ = new (alloc_type ()) write_queue_type ();

    schedule ();
    scheduler_->finish<init_traits> ();
  }

  // Info request.
  void
  info_request (aid_t aid)
  {
    // Record the request for information.
    info_queue_->push_back (aid);
    schedule ();
    scheduler_->finish<info_request_traits> ();
  }

  // Info response.
  void
  info_response (aid_t aid)
  {
    scheduler_->remove<info_response_traits> (&info_response, aid);
    if (!info_queue_->empty () && aid == info_queue_->front ()) {
      info_queue_->pop_front ();
      schedule ();
      scheduler_->finish<info_response_traits> (&info_);
    }
    else {
      schedule ();
      scheduler_->finish<info_response_traits> ();
    }
  }

  // Read request.
  void
  read_request (aid_t aid,
		read_request_t request)
  {
    read_queue_->push_back (read_queue_item_type (aid, request));
    schedule ();
    scheduler_->finish<read_request_traits> ();
  }

  // Read response.
  void
  read_response (aid_t aid)
  {
    scheduler_->remove<read_response_traits> (&read_response, aid);
    if (!read_queue_->empty () && aid == read_queue_->front ().first) {
      block_num_t block_num = read_queue_->front ().second.block_num;
      read_queue_->pop_front ();
      if (block_num < info_.block_count) {
	// Succeed.
	read_response_t response (block_num, SUCCESS);
	schedule ();
	scheduler_->finish<read_response_traits> (syscall::buffer_copy (bid, response.block_num * pagesize_, pagesize_), &response);
      }
      else {
	// Succeed.
	read_response_t response (block_num, RANGE);
	schedule ();
	scheduler_->finish<read_response_traits> (syscall::buffer_create (0), &response);
      }
    }
    else {
      schedule ();
      scheduler_->finish<read_response_traits> ();
    }
  }

  // Write request.
  void
  write_request (aid_t aid,
		 bid_t b,
		 write_request_t request)
  {
    if (syscall::buffer_size (b) == pagesize_ && request.block_num < info_.block_count) {
      // Succeed.
      syscall::buffer_assign (bid, request.block_num * pagesize_, b, 0, pagesize_);
      write_queue_->push_back (write_queue_item_type (aid, write_response_t (request.block_num, SUCCESS)));
    }
    else if (syscall::buffer_size (b) != pagesize_) {
      write_queue_->push_back (write_queue_item_type (aid, write_response_t (request.block_num, BUFFER_SIZE)));
    }
    else if (request.block_num < info_.block_count) {
      write_queue_->push_back (write_queue_item_type (aid, write_response_t (request.block_num, RANGE)));
    }

    schedule ();
    scheduler_->finish<write_request_traits> ();
  }
  
  // Write response.
  void
  write_response (aid_t aid)
  {
    scheduler_->remove<write_response_traits> (&write_response, aid);
    if (!write_queue_->empty () && aid == write_queue_->front ().first) {
      write_response_t response = write_queue_->front ().second;
      write_queue_->pop_front ();
      schedule ();
      scheduler_->finish<write_response_traits> (&response);
    }
    else {
      schedule ();
      scheduler_->finish<write_response_traits> ();
    }
  }
}
