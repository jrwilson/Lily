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

    bid_t b1 = system::buffer_create (2 * system::getpagesize ());
    kassert (b1 != -1);
    kassert (system::buffer_size (b1) == 2 * system::getpagesize ());
    char* c = static_cast<char*> (system::buffer_map (b1));
    kassert (c != reinterpret_cast<void*> (-1));
    memset (c, 0xFF, system::getpagesize ());

    for (size_t i = 0; i < system::getpagesize (); ++i) {
      kassert (c[i] == static_cast<char> (0xFF));
    }

    for (size_t i = system::getpagesize (); i < 2 * system::getpagesize (); ++i) {
      kassert (c[i] == 0x00);
    }

    for (size_t i = system::getpagesize (); i < 2 * system::getpagesize (); ++i) {
      c[i] = 0xFF;
    }

    bid_t b2 = system::buffer_create (system::getpagesize ());
    kassert (b2 != -1);
    kassert (system::buffer_append (b2, b1) == 0);
    char* d = static_cast<char*> (system::buffer_map (b2));
    kassert (d != reinterpret_cast<void*> (-1));
    memset (d, 0xAB, system::buffer_size (b2));

    for (size_t i = 0; i < system::buffer_size (b1); ++i) {
      kassert (c[i] == static_cast<char> (0xFF));
    }

    for (size_t i = 0; i < system::buffer_size (b2); ++i) {
      kassert (d[i] == static_cast<char> (0xAB));
    }

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
