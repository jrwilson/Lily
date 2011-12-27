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

    const size_t pagesize = syscall::getpagesize ();

    {
      // Q: Can we create an empty buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we map an empty buffer?
      // A: No.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_map (b) == reinterpret_cast<const void*> (-1));
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we add bytes at the end of a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_grow (b, 0) == 0);
      kassert (syscall::buffer_grow (b, 1) == 0);
      kassert (syscall::buffer_size (b) == pagesize);
      kassert (syscall::buffer_grow (b, 1) == pagesize);
      kassert (syscall::buffer_size (b) == 2 * pagesize);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we add bytes at the end of a buffer that has been mapped?
      // A: No.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_grow (b, 0) == 0);
      kassert (syscall::buffer_grow (b, 1) == 0);
      kassert (syscall::buffer_size (b) == pagesize);
      kassert (syscall::buffer_map (b) != reinterpret_cast<const void*> (-1));
      kassert (syscall::buffer_grow (b, 1) == size_t (-1));
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we create a non-empty buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create (1);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == pagesize);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we map a non-empty buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create (1);
      kassert (b != -1);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      memset (c, 'A', pagesize);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we map a buffer twice?
      // A: No.  The address is the same.
      bid_t b = syscall::buffer_create (1);
      kassert (b != -1);
      char* c1 = static_cast<char*> (syscall::buffer_map (b));
      kassert (c1 != reinterpret_cast<const void*> (-1));
      char* c2 = static_cast<char*> (syscall::buffer_map (b));
      kassert (c1 == c2);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: What happens if I try to destroy a buffer that doesn't exist?
      // A: Returns -1.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_destroy (b) == 0);
      kassert (syscall::buffer_destroy (b) == -1);
    }

    {
      // Q: What is the size of a buffer that doesn't exist?
      // A: The equivalent of -1.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_destroy (b) == 0);
      kassert (syscall::buffer_size (b) == size_t (-1));
    }

    {
      // Q: What happens if I try to map a buffer that doesn't exist?
      // A: Returns -1.
      bid_t b = syscall::buffer_create (1);
      kassert (b != -1);
      kassert (syscall::buffer_destroy (b) == 0);
      kassert (syscall::buffer_map (b) == reinterpret_cast<const void*> (-1));
    }

    // Create a buffer for testing.
    const size_t abcd_size = 4 * pagesize;
    const bid_t abcd_b = syscall::buffer_create (abcd_size);
    kassert (abcd_b != -1);
    char* abcd_c = static_cast<char*> (syscall::buffer_map (abcd_b));
    memset (abcd_c + 0 * pagesize, 'A', pagesize);
    memset (abcd_c + 1 * pagesize, 'B', pagesize);
    memset (abcd_c + 2 * pagesize, 'C', pagesize);
    memset (abcd_c + 3 * pagesize, 'D', pagesize);

    {
      // Q: Can I copy a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_copy (abcd_b);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == abcd_size);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      kassert (memcmp (abcd_c, c, abcd_size) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I copy a buffer starting at a certain offset?
      // A: Yes.  The offset will be page aligned.
      bid_t b = syscall::buffer_copy (abcd_b, pagesize);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == abcd_size - pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      kassert (memcmp (abcd_c + pagesize, c, abcd_size - pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I copy a buffer starting at a certain offset going a certain length?
      // A: Yes.  The offset and length will be page aligned.
      bid_t b = syscall::buffer_copy (abcd_b, pagesize, 1);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      kassert (memcmp (abcd_c + pagesize, c, pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b) == 0);
      kassert (syscall::buffer_size (b) == abcd_size);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      kassert (memcmp (abcd_c, c, abcd_size) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append a buffer starting at a certain offset?
      // A: Yes.  The offset will be page aligned.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b, pagesize) == 0);
      kassert (syscall::buffer_size (b) == abcd_size - pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      kassert (memcmp (abcd_c + pagesize, c, abcd_size - pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append a buffer starting at a certain offset going a certain length?
      // A: Yes.  The offset and length will be page aligned.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b, pagesize, 1) == 0);
      kassert (syscall::buffer_size (b) == pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      kassert (memcmp (abcd_c + pagesize, c, pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append to a buffer that is mapped?
      // A: No.
      bid_t b = syscall::buffer_create ();
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b) == 0);
      kassert (syscall::buffer_size (b) == abcd_size);
      kassert (syscall::buffer_map (b) != reinterpret_cast<const void*> (-1));
      kassert (syscall::buffer_append (b, abcd_b) == size_t (-1));
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I change the contents of a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_copy (abcd_b);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == abcd_size);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != reinterpret_cast<const void*> (-1));
      memset (c, 'E', abcd_size);
      // The copied buffer doesn't change.
      for (size_t idx = 0; idx < pagesize; ++idx) {
	kassert (abcd_c[idx + 0 * pagesize] == 'A');
	kassert (abcd_c[idx + 1 * pagesize] == 'B');
	kassert (abcd_c[idx + 2 * pagesize] == 'C');
	kassert (abcd_c[idx + 3 * pagesize] == 'D');
      }
      kassert (syscall::buffer_destroy (b) == 0);
    }

    kassert (syscall::buffer_destroy (abcd_b) == 0);

    schedule ();
    scheduler_->finish<init_traits> ();
  }
}
