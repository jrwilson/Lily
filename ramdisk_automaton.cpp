#include "ramdisk_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kout.hpp"

struct allocator_tag { };
typedef list_alloc<allocator_tag> alloc_type;

template <class T>
typename list_alloc<T>::data list_alloc<T>::data_;

template <typename T>
struct allocator_type : public list_allocator<T, allocator_tag> { };

namespace ramdisk {

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  // The buffer to serve.
  bid_t bid;

  static info_response_t info_;
  
  // Queues for processing requests.
  typedef std::deque<aid_t, allocator_type<aid_t> > info_queue_type;
  static info_queue_type* info_queue_;

  typedef std::pair<aid_t, read_request_t> read_queue_item_type;
  typedef std::deque<read_queue_item_type, allocator_type<read_queue_item_type> > read_queue_type;
  static read_queue_type* read_queue_;

  typedef std::pair<aid_t, write_request_t> write_queue_item_type;
  typedef std::deque<write_queue_item_type, allocator_type<write_queue_item_type> > write_queue_type;
  static write_queue_type* write_queue_;

  static void
  schedule ()
  { }

  // Init.
  void
  init (void)
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();
    // Calculate the number of blocks.
    info_.block_count = system::buffer_size (bid) / system::getpagesize ();
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
    if (aid == info_queue_->front ()) {
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
  read_response (aid_t)
  {
    // TODO
    schedule ();
    scheduler_->finish<read_response_traits> ();
  }

  // Write request.
  void
  write_request (aid_t,
		 bid_t,
		 write_request_t)
  {
    // TODO
    schedule ();
    scheduler_->finish<write_request_traits> ();
  }
  
  // Write response.
  void
  write_response (aid_t)
  {
    // TODO
    schedule ();
    scheduler_->finish<write_response_traits> ();
  }
}
