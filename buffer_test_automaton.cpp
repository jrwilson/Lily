#include "buffer_test_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kassert.hpp"
#include <string.h>

namespace buffer_test {

  static list_alloc_state state_;

  struct buffer_test_automaton_syscall : public list_alloc_syscall {
    static list_alloc_state&
    state ()
    {
      return state_;
    }
  };
  
  typedef list_alloc<buffer_test_automaton_syscall> alloc_type;
  
  template <typename T>
  struct allocator_type : public list_allocator<T, buffer_test_automaton_syscall> { };
  
  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static void
  schedule ()
  { }
  
  void
  init ()
  {
    kout << "buffer_test " << __func__ << endl;

    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();

    const size_t pagesize = syscall::getpagesize ();

    {
      // Q: Can we create an empty buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we map an empty buffer?
      // A: No.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_map (b) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we add bytes at the end of a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create (0);
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
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_grow (b, 0) == 0);
      kassert (syscall::buffer_grow (b, 1) == 0);
      kassert (syscall::buffer_size (b) == pagesize);
      kassert (syscall::buffer_map (b) != 0);
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
      kassert (c != 0);
      memset (c, 'A', pagesize);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can we map a buffer twice?
      // A: No.  The address is the same.
      bid_t b = syscall::buffer_create (1);
      kassert (b != -1);
      char* c1 = static_cast<char*> (syscall::buffer_map (b));
      kassert (c1 != 0);
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
      kassert (syscall::buffer_map (b) == 0);
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
      bid_t b = syscall::buffer_copy (abcd_b, 0, abcd_size);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == abcd_size);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
      kassert (memcmp (abcd_c, c, abcd_size) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I copy a buffer starting at a certain offset?
      // A: Yes.  The offset will be page aligned.
      bid_t b = syscall::buffer_copy (abcd_b, pagesize, abcd_size - pagesize);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == abcd_size - pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
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
      kassert (c != 0);
      kassert (memcmp (abcd_c + pagesize, c, pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b, 0, abcd_size) == 0);
      kassert (syscall::buffer_size (b) == abcd_size);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
      kassert (memcmp (abcd_c, c, abcd_size) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append a buffer starting at a certain offset?
      // A: Yes.  The offset will be page aligned.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b, pagesize, abcd_size - pagesize) == 0);
      kassert (syscall::buffer_size (b) == abcd_size - pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
      kassert (memcmp (abcd_c + pagesize, c, abcd_size - pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append a buffer starting at a certain offset going a certain length?
      // A: Yes.  The offset and length will be page aligned.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b, pagesize, 1) == 0);
      kassert (syscall::buffer_size (b) == pagesize);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
      kassert (memcmp (abcd_c + pagesize, c, pagesize) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I append to a buffer that is mapped?
      // A: No.
      bid_t b = syscall::buffer_create (0);
      kassert (b != -1);
      kassert (syscall::buffer_append (b, abcd_b, 0, abcd_size) == 0);
      kassert (syscall::buffer_size (b) == abcd_size);
      kassert (syscall::buffer_map (b) != 0);
      kassert (syscall::buffer_append (b, abcd_b, 0, abcd_size) == size_t (-1));
      kassert (syscall::buffer_destroy (b) == 0);
    }

    {
      // Q: Can I change the contents of a buffer?
      // A: Yes.
      bid_t b = syscall::buffer_copy (abcd_b, 0, abcd_size);
      kassert (b != -1);
      kassert (syscall::buffer_size (b) == abcd_size);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
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

    {
      // Q: Can I assign parts of one buffer to another?
      // A: Yes.  The source/destination must exist and can be mapped.
      bid_t b = syscall::buffer_create (4 * pagesize);
      kassert (b != -1);
      char* c = static_cast<char*> (syscall::buffer_map (b));
      kassert (c != 0);
      kassert (syscall::buffer_assign (b, pagesize, abcd_b, pagesize, 2 * pagesize) == 0);
      kassert (syscall::buffer_assign (b, 0, abcd_b, 0, 2 * pagesize) == 0);
      kassert (syscall::buffer_assign (b, 3 * pagesize, abcd_b, 3 * pagesize, pagesize) == 0);
      kassert (memcmp (c, abcd_c, abcd_size) == 0);
      kassert (syscall::buffer_destroy (b) == 0);
    }

    kassert (syscall::buffer_destroy (abcd_b) == 0);

    schedule ();
    scheduler_->finish<init_traits> ();
  }
}
