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
  schedule ()
  { }
  
  void
  init ()
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();

    // kassert (syscall::buffer_size (85) == static_cast<size_t> (-1));

    // bid_t b1 = syscall::buffer_create (2 * syscall::getpagesize ());
    // kassert (b1 != -1);
    // kassert (syscall::buffer_size (b1) == 2 * syscall::getpagesize ());
    // char* c = static_cast<char*> (syscall::buffer_map (b1));
    // kassert (c != reinterpret_cast<void*> (-1));
    // memset (c, 0xFF, syscall::getpagesize ());

    // for (size_t i = 0; i < syscall::getpagesize (); ++i) {
    //   kassert (c[i] == static_cast<char> (0xFF));
    // }

    // for (size_t i = syscall::getpagesize (); i < 2 * syscall::getpagesize (); ++i) {
    //   kassert (c[i] == 0x00);
    // }

    // for (size_t i = syscall::getpagesize (); i < 2 * syscall::getpagesize (); ++i) {
    //   c[i] = 0xFF;
    // }

    // bid_t b2 = syscall::buffer_create (syscall::getpagesize ());
    // kassert (b2 != -1);
    // syscall::buffer_append (b2, b1);
    // char* d = static_cast<char*> (syscall::buffer_map (b2));
    // kassert (d != reinterpret_cast<void*> (-1));
    // memset (d, 0xAB, syscall::buffer_size (b2));

    // for (size_t i = 0; i < syscall::buffer_size (b1); ++i) {
    //   kassert (c[i] == static_cast<char> (0xFF));
    // }

    // for (size_t i = 0; i < syscall::buffer_size (b2); ++i) {
    //   kassert (d[i] == static_cast<char> (0xAB));
    // }

    schedule ();
    scheduler_->finish<init_traits> ();
  }
}
