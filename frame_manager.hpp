#ifndef __frame_manager_hpp__
#define __frame_manager_hpp__

/*
  File
  ----
  frame_manager.hpp
  
  Description
  -----------
  The frame manager manages page-sized units of physical memory called frames.
  Frames are reference counted so that frames can be shared.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include "kput.hpp"
#include "multiboot.hpp"
#include <vector>
#include <algorithm>
#include "vm_def.hpp"
#include "stack_allocator.hpp"
#include "system_allocator.hpp"

/*
  The frame manager was designed under the following requirements and assumptions:
  1.  All frames can be shared.
  2.  The physical location of a frame doesn't matter.

  Sharing frames implies reference counting.

  The assumption that all frames can be shared is somewhat naive as only frames containing code and buffers can be shared.
  Thus, an optimization is to assume that only a fixed fraction of frames are shared.

  The assumption that the physical location of a frame doesn't matter will probably break down once we start doing DMA.
  However, this should only require expanding the interface and changing the implementation as opposed to redesigning the interface.
 */

class frame_manager {
private:
  typedef std::vector<stack_allocator*, system_allocator<stack_allocator*> > allocator_list_type;
  static allocator_list_type allocator_list_;

  struct contains_frame {
    frame_t f;

    contains_frame (frame_t x) :
      f (x)
    { }

    bool
    operator() (stack_allocator* const & sa) const
    {
      return f >= sa->begin () && f < sa->end ();
    }
  };

  static inline allocator_list_type::iterator
  find_allocator (frame_t frame)
  {
    // TODO:  Sort the stack allocators and use binary search.
    return std::find_if (allocator_list_.begin (), allocator_list_.end (), contains_frame (frame));
  }

  struct stack_allocator_not_full {
    bool
    operator() (stack_allocator* const& sa) const
    {
      return !sa->full ();
    }
  };

  frame_manager ();
  frame_manager (const frame_manager&);
  frame_manager& operator= (const frame_manager&);

public:
  template <class InputIterator>
  static void
  initialize (InputIterator begin,
	      InputIterator end)
  {
    for (; begin != end; ++begin) {
      kputx64 (begin->addr); kputs ("-"); kputx64 (begin->addr + begin->len - 1);
      switch (begin->type) {
      case MULTIBOOT_MEMORY_AVAILABLE:
	kputs (" AVAILABLE\n");
	break;
      case MULTIBOOT_MEMORY_RESERVED:
	kputs (" RESERVED\n");
	break;
      default:
	kputs (" UNKNOWN\n");
	break;
      }

      if (begin->type == MULTIBOOT_MEMORY_AVAILABLE) {
      	physical_address_t b = std::max (static_cast<multiboot_uint64_t> (USABLE_MEMORY_BEGIN), begin->addr);
      	physical_address_t e = std::min (static_cast<multiboot_uint64_t> (USABLE_MEMORY_END), begin->addr + begin->len);
	
      	// Align to frame boundaries.
      	b = align_up (b, PAGE_SIZE);
      	e = align_down (e, PAGE_SIZE);

      	if (b < e) {
      	  size_t size = e - b;
      	  while (size != 0) {
      	    size_t sz = std::min (stack_allocator::MAX_REGION_SIZE, size);
	    allocator_list_.push_back (new (system_alloc ()) stack_allocator (physical_address_to_frame (b), physical_address_to_frame (b + size)));
      	    size -= sz;
      	    b += sz;
      	  }
      	}
      }
    }
  }
  
  /* This function allows a frame to be marked as used when initializing virtual memory. */
  static void
  mark_as_used (frame_t frame);
  
  /* Allocate a frame. */
  static frame_t
  alloc () __attribute__((warn_unused_result));
  
  /* Increment the reference count for a frame. */
  static size_t
  incref (frame_t frame);
  
  /* Decrement the reference count for a frame. */
  static size_t
  decref (frame_t frame);
};


#endif /* __frame_manager_hpp__ */
