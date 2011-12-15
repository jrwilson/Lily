#include "buffer_test_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kassert.hpp"

struct allocator_tag { };
typedef list_alloc<allocator_tag> alloc_type;

template <class T>
typename list_alloc<T>::data list_alloc<T>::data_;

template <typename T>
struct allocator_type : public list_allocator<T, allocator_tag> { };

namespace buffer_test {

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static void
  schedule ();
  
  void
  init ()
  {
    kout << __func__ << endl;
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();

    bid_t buffer = system::buffer_create (100);
    kout << "create       " << buffer << endl;
    kout << "size         " << system::buffer_size (buffer) << endl;
    kout << "size (bad)   " << system::buffer_size (85) << endl;
    kout << "incref       " << system::buffer_incref (buffer) << endl;
    kout << "incref (bad) " << system::buffer_incref (85) << endl;
    kout << "decref       " << system::buffer_decref (buffer) << endl;
    kout << "decref (bad) " << system::buffer_decref (85) << endl;

    schedule ();
    scheduler_->finish<init_traits> ();
  }

  static void
  no_schedule () { }

  static void
  no_finish () {
    system::finish (0, 0, false, 0);
  }

  static void
  schedule ()
  {

  }

}
