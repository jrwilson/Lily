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

#include "stack_allocator.hpp"
#include "vector.hpp"

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
public:
  static void
  add (physical_address_t begin,
       physical_address_t end);
  
  /* This function allows a frame to be marked as used when initializing virtual memory. */
  static void
  mark_as_used (frame_t frame);
  
  /* Allocate a frame. */
  static inline frame_t
  alloc ()
  {
    /* Find an allocator with a free frame. */
    allocator_list_type::iterator pos = find_if (allocator_list_.begin (), allocator_list_.end (), stack_allocator_not_full ());
    
    /* Out of frames. */
    kassert (pos != allocator_list_.end ());
  
    return (*pos)->alloc ();
  }
  
  /* Increment the reference count for a frame. */
  static inline size_t
  incref (frame_t frame,
	  size_t count = 1)
  {
    allocator_list_type::iterator pos = find_allocator (frame);
    
    /* No allocator for frame. */
    kassert (pos != allocator_list_.end ());
    
    return (*pos)->incref (frame, count);
  }
  
  /* Decrement the reference count for a frame. */
  static inline size_t
  decref (frame_t frame)
  {
    allocator_list_type::iterator pos = find_allocator (frame);
    
    /* No allocator for frame. */
    kassert (pos != allocator_list_.end ());
    
    return (*pos)->decref (frame);
  }

private:
  typedef vector<stack_allocator*> allocator_list_type;
  static allocator_list_type allocator_list_;

  struct contains_frame {
    frame_t f;

    contains_frame (frame_t x) :
      f (x)
    { }

    inline bool
    operator() (stack_allocator* const & sa) const
    {
      return f >= sa->begin () && f < sa->end ();
    }
  };

  static inline allocator_list_type::iterator
  find_allocator (frame_t frame)
  {
    // TODO:  Sort the stack allocators and use binary search.
    return find_if (allocator_list_.begin (), allocator_list_.end (), contains_frame (frame));
  }

  struct stack_allocator_not_full {
    inline bool
    operator() (stack_allocator* const& sa) const
    {
      return !sa->full ();
    }
  };
};

#endif /* __frame_manager_hpp__ */
