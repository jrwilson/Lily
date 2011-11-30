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

#include "placement_allocator.hpp"
#include "types.hpp"
#include "frame.hpp"
#include "kput.hpp"
#include "multiboot.hpp"
#include <algorithm>
#include <vector>
#include "vm_def.hpp"
#include "stack_allocator.hpp"

/* Don't mess with memory below 1M or above 4G. */
static const physical_address USABLE_MEMORY_BEGIN (0x00100000);
static const physical_address USABLE_MEMORY_END (0xFFFFF000);

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
  typedef std::vector<stack_allocator*, placement_allocator<stack_allocator*> > stack_allocators_type;
  stack_allocators_type stack_allocators_;

  struct counter {
    size_t count;

    counter () :
      count (0)
    { }

    void
    operator() (const physical_address&,
		size_t&)
    {
      ++count;
    }
  };

  struct allocate {
    size_t idx;
    stack_allocators_type& stack_allocators;
    placement_alloc& alloc;

    allocate (stack_allocators_type& sa,
	      placement_alloc& a) :
      idx (0),
      stack_allocators (sa),
      alloc (a)
    { }

    void
    operator() (const physical_address& begin,
		size_t& size)
    {
      stack_allocators[idx++] = new (alloc) stack_allocator (frame (begin), frame (begin + size), alloc);
    }
  };

  template <class T>
  struct stack_allocator_filter {
    T value;

    stack_allocator_filter (const T& t = T ()) :
      value (t)
    { }
    
    void
    operator() (const multiboot_memory_map_t& entry)
    {
      if (entry.type == MULTIBOOT_MEMORY_AVAILABLE) {
	physical_address begin (std::max (static_cast<multiboot_uint64_t> (USABLE_MEMORY_BEGIN.value ()), entry.addr));
	physical_address end (std::min (static_cast<multiboot_uint64_t> (USABLE_MEMORY_END.value ()), entry.addr + entry.len));
	
	// Align to frame boundaries.
	begin <<= PAGE_SIZE;
	end >>= PAGE_SIZE;

	if (begin < end) {
	  size_t size = end - begin;
	  while (size != 0) {
	    size_t sz = std::min (stack_allocator::MAX_REGION_SIZE, size);
	    value (begin, sz);
	    size -= sz;
	    begin += sz;
	  }
	}
      }
    }
  };

  struct print_mmap {
    void
    operator() (const multiboot_memory_map_t& entry)
    {
      kputx64 (entry.addr); kputs ("-"); kputx64 (entry.addr + entry.len - 1);
      switch (entry.type) {
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
    }
  };

  struct contains_frame {
    frame f;

    contains_frame (frame x) :
      f (x)
    { }

    bool
    operator() (stack_allocator* const & sa) const
    {
      return f >= sa->begin () && f < sa->end ();
    }
  };

  inline stack_allocators_type::iterator
  find_allocator (frame frame)
  {
    // TODO:  Sort the stack allocators and use binary search.
    return std::find_if (stack_allocators_.begin (), stack_allocators_.end (), contains_frame (frame));
  }

  inline stack_allocators_type::const_iterator
  find_allocator (frame frame) const
  {
    // TODO:  Sort the stack allocators and use binary search.
    return std::find_if (stack_allocators_.begin (), stack_allocators_.end (), contains_frame (frame));
  }

  struct stack_allocator_not_full {
    bool
    operator() (stack_allocator* const& sa) const
    {
      return !sa->full ();
    }
  };

public:
  template <class InputIterator>
  frame_manager (InputIterator begin,
		 InputIterator end,
		 placement_alloc& a) :
    stack_allocators_ (std::for_each (begin, end, stack_allocator_filter<counter> ()).value.count, 0, stack_allocators_type::allocator_type (a))
  {
    std::for_each (begin, end, stack_allocator_filter<allocate> (allocate (stack_allocators_, a)));
    std::for_each (begin, end, print_mmap ());
  }
  
  /* This function allows a frame to be marked as used when initializing virtual memory. */
  void
  mark_as_used (const frame& frame);
  
  /* Allocate a frame. */
  frame
  alloc () __attribute__((warn_unused_result));
  
  /* Increment the reference count for a frame. */
  void
  incref (const frame& frame);
  
  // /* Decrement the reference count for a frame. */
  // void
  // decref (const frame& frame);
};


#endif /* __frame_manager_hpp__ */
