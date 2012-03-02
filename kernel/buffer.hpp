#ifndef __buffer_hpp__
#define __buffer_hpp__

#include "vm_area.hpp"
#include "vector.hpp"

// TODO:  Don't increment/decrement the zero frame.

/*
  Buffer
  ======
  Buffers are arrays of frames used to move data between automata.
  Output actions generate buffers that are copied to input actions.
  The actual data is not copied, rather, the original buffer is put in a copy-on-write state and new buffers that reference the actual data are created.
  Buffers are local to an automaton.
  Buffers can also be used as a form of dynamic memory.
  
  The main operations on a buffer are as follows:
  create - create a buffer
  copy - create a buffer from part of another buffer
  destroy - destroy a buffer
  size - return the size of the buffer
  resize - change the size of the buffer
  assign - replace entries in a buffer with entries from another (or the same) buffer
  append - extend the buffer with part of a buffer
  map - map the buffer into an automaton's address space
  unmap - remove the buffer from an automaton's address space

  Operations like resize and append require that the destination buffer not be mapped.

  As the frames in a buffer are changed by writing and the copy-on-write system, the internal list of frames no longer resembles the buffer.
  The synchronization operation makes the internal list of frames resemble the actual frames and places them in a copy-on-write state.

  One important issue is whether sizes and offsets refer to frames or whether they refer to bytes.
  Initially, I implemented buffer operations in terms of bytes but I found this to be misleading for operations like copy, assign, and append because of the implicit rounding to page boundaries.
  The downsize of expressing sizes and offsets in frames is that is forces the user to scale by page size, a possible source of errors.
  However, expressing sizes and offsets in frames makes the semantics of buffers more obvious.
  Thus, offsets and sizes are expressed in terms of frames.
*/

class buffer : public vm_area_base {
public:
  buffer (size_t size) :
    vm_area_base (0, 0),
    frame_list_ (size, vm::zero_frame ())
  {
    frame_manager::incref (vm::zero_frame (), frame_list_.size ());
  }
  
  buffer (buffer& other,
	  size_t begin,
	  size_t end) :
    vm_area_base (0, 0)
  {
    other.sync (begin, end);
    frame_list_.insert (frame_list_.end (), other.frame_list_.begin () + begin, other.frame_list_.begin () + end);
    for(frame_list_type::const_iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      frame_manager::incref (*pos);
    }
  }

  // Used for output/input copying.
  // other should be synchronized before this call.
  buffer (const buffer& other) :
    vm_area_base (0, 0),
    frame_list_ (other.frame_list_)
  {
    for(frame_list_type::const_iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      frame_manager::incref (*pos);
    }
  }

  ~buffer ()
  {
    unmap ();

    // Free the frames.
    for (frame_list_type::const_iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      frame_manager::decref (*pos);
    }
  }

  void
  map_begin (logical_address_t begin)
  {
    kassert (begin_ == 0);
    begin_ = align_down (begin, PAGE_SIZE);
    end_ = begin_ + frame_list_.size () * PAGE_SIZE;

    for (size_t idx = 0; idx != frame_list_.size (); ++idx) {
      /* Do not increment the reference count. */
      vm::map (begin_ + idx * PAGE_SIZE, frame_list_[idx], vm::USER, vm::MAP_COPY_ON_WRITE, false, vm::BUFFER);
    }
  }

  void
  map_end (logical_address_t end)
  {
    if (begin_ == 0) {
      end_ = align_down (end, PAGE_SIZE);
      begin_ = end_ - frame_list_.size () * PAGE_SIZE;
      
      for (size_t idx = 0; idx != frame_list_.size (); ++idx) {
	/* Do not increment the reference count. */
	vm::map (begin_ + idx * PAGE_SIZE, frame_list_[idx], vm::USER, vm::MAP_COPY_ON_WRITE, false, vm::BUFFER);
      }
    }
  }

  void
  unmap ()
  {
    if (begin_ != 0) {
      sync (0, frame_list_.size ());

      for (size_t idx = 0; idx != frame_list_.size (); ++idx) {
	/* Do not decrement the reference count. */
	vm::unmap (begin_ + idx * PAGE_SIZE, false);
      }

      begin_ = 0;
      end_ = 0;
    }
  }

  void
  override (logical_address_t begin,
	    logical_address_t end)
  {
    begin_ = begin;
    end_ = end;
  }

  size_t
  size () const
  {
    return frame_list_.size ();
  }

  void
  resize (size_t size)
  {
    // Cannot be mapped.
    kassert (begin_ == 0);

    size_t old_size = frame_list_.size ();
    
    if (size < old_size) {
      /* Shrink. */
      while (frame_list_.size () != size) {
	frame_manager::decref (frame_list_.back ());
	frame_list_.pop_back ();
      }
    }
    else if (size > old_size) {
      frame_list_.resize (size, vm::zero_frame ());
      frame_manager::incref (vm::zero_frame (), size - old_size);
    }
  }

  size_t
  append (buffer& other,
	  size_t begin,
	  size_t end)
  {
    // Cannot be mapped.
    kassert (begin_ == 0);
    other.sync (begin, end);
    size_t old_size = frame_list_.size ();
    frame_list_.insert (frame_list_.end (), other.frame_list_.begin () + begin, other.frame_list_.begin () + end);
    for (size_t idx = old_size; idx != frame_list_.size (); ++idx) {
      frame_manager::incref (frame_list_[idx]);
    }
    return old_size;
  }

  void
  assign (size_t dst_begin,
	  buffer& src,
	  size_t src_begin,
	  size_t src_end)
  {
    src.sync (src_begin, src_end);

    for (size_t idx = 0; idx != (src_end - src_begin); ++idx) {
      if (begin_ != 0) {
	// Unmap.
	vm::unmap (begin_ + (dst_begin + idx) * PAGE_SIZE, false);
      }
      frame_manager::decref (frame_list_[dst_begin + idx]);
      frame_list_[dst_begin + idx] = src.frame_list_[src_begin + idx];
      frame_manager::incref (frame_list_[dst_begin + idx]);
      if (begin_ != 0) {
	// Map.
	vm::map (begin_ + (dst_begin + idx) * PAGE_SIZE, frame_list_[dst_begin + idx], vm::USER, vm::MAP_COPY_ON_WRITE, false, vm::BUFFER);
      }
    }
  }

  void
  append_frame (frame_t frame)
  {
    // Not mapped.
    kassert (begin_ == 0);

    frame_list_.push_back (frame);
    frame_manager::incref (frame);
  }

  // Synchronize part of a buffer making it copy-on-write.
  void
  sync (size_t begin,
	size_t end)
  {
    if (begin_ != 0) {
      for (; begin != end; ++begin) {
	frame_t actual = vm::logical_address_to_frame (begin_ + begin * PAGE_SIZE);
	if (frame_list_[begin] != actual) {
	  // The frame changed.  Its reference count must be 1.
	  kassert (frame_manager::ref_count (actual) == 1);
	  // Decrement the reference for the old frame.
	  frame_manager::decref (frame_list_[begin]);
	  frame_list_[begin] = actual;
	  // We don't need to increment the reference count for the new frame as it should be 1.
	  vm::remap (begin_ + begin * PAGE_SIZE, vm::USER, vm::MAP_COPY_ON_WRITE, vm::BUFFER);
	}
      }
    }
  }

private:
  // The frames.
  typedef vector<frame_t> frame_list_type;
  frame_list_type frame_list_;
};

#endif /* __buffer_hpp__ */
