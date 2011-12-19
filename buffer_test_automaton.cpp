#include "buffer_test_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kassert.hpp"
#include <string.h>

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

    kassert (system::buffer_size (85) == static_cast<size_t> (-1));
    kassert (system::buffer_incref (85) == -1);
    kassert (system::buffer_decref (85) == -1);

    bid_t buffer = system::buffer_create (2 * system::getpagesize ());
    kassert (system::buffer_size (buffer) == 2 * system::getpagesize ());
    char* c = static_cast<char*> (system::buffer_map (buffer));
    kassert (c != reinterpret_cast<void*> (-1));
    memset (c, 0xFF, system::getpagesize ());
    kassert (system::buffer_incref (buffer) == 2);
    kassert (system::buffer_decref (buffer) == 1);


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
