#ifndef __buffer_hpp__
#define __buffer_hpp__

#include "vm_area.hpp"

// TODO:  Don't increment/decrement the zero frame.

class buffer : public vm_area_base {
public:
  buffer (size_t size) :
    vm_area_base (0, 0),
    frame_list_ (size / PAGE_SIZE, vm::zero_frame ())
  {
    frame_manager::incref (vm::zero_frame (), frame_list_.size ());
  }
  
  buffer (buffer& other,
	  size_t offset,
	  size_t length) :
    vm_area_base (0, 0)
  {
    other.sync (offset, length);
    frame_list_.insert (frame_list_.end (), other.frame_list_.begin () + offset / PAGE_SIZE, other.frame_list_.begin () + (offset + length) / PAGE_SIZE);
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
  map (logical_address_t end)
  {
    if (begin_ == 0) {
      end_ = align_down (end, PAGE_SIZE);
      begin_ = end_ - size ();
      
      for (size_t idx = 0; idx != frame_list_.size (); ++idx) {
	vm::map (begin_ + idx * PAGE_SIZE, frame_list_[idx], vm::USER, vm::MAP_COPY_ON_WRITE);
      }
    }
  }

  void
  unmap ()
  {
    if (begin_ != 0) {
      sync (0, size ());

      for (size_t idx = 0; idx != frame_list_.size (); ++idx) {
	vm::unmap (begin_ + idx * PAGE_SIZE);
      }

      begin_ = 0;
      end_ = 0;
    }
  }

  size_t
  size () const
  {
    return frame_list_.size () * PAGE_SIZE;
  }

  size_t
  grow (size_t size)
  {
    // Cannot be mapped.
    kassert (begin_ == 0);
    size_t retval = this->size ();
    size_t begin = frame_list_.size ();
    frame_list_.resize (begin + size / PAGE_SIZE, vm::zero_frame ());
    frame_manager::incref (vm::zero_frame (), frame_list_.size () - begin);
    return retval;
  }

  size_t
  append (buffer& other,
	  size_t offset,
	  size_t length)
  {
    // Cannot be mapped.
    kassert (begin_ == 0);
    other.sync (offset, length);
    size_t retval = this->size ();
    size_t begin = frame_list_.size ();
    frame_list_.insert (frame_list_.end (), other.frame_list_.begin () + offset / PAGE_SIZE, other.frame_list_.begin () + (offset + length) / PAGE_SIZE);
    for (; begin != frame_list_.size (); ++begin) {
      frame_manager::incref (frame_list_[begin]);
    }
    return retval;
  }

  void
  assign (size_t offset,
	  buffer& src,
	  size_t src_offset,
	  size_t length)
  {
    src.sync (src_offset, length);

    offset /= PAGE_SIZE;
    src_offset /= PAGE_SIZE;
    length /= PAGE_SIZE;

    for (size_t idx = 0; idx != length; ++idx) {
      if (src.begin_ != 0) {
	// Remap copy on write.
	vm::remap (src.begin_ + (src_offset + idx) * PAGE_SIZE, vm::USER, vm::MAP_COPY_ON_WRITE);
      }
      if (begin_ != 0) {
	// Unmap.
	vm::unmap (begin_ + (offset + idx) * PAGE_SIZE);
      }
      frame_manager::decref (frame_list_[offset + idx]);
      frame_list_[offset + idx] = src.frame_list_[src_offset + idx];
      frame_manager::incref (frame_list_[offset + idx]);
      if (begin_ != 0) {
	// Map.
	vm::map (begin_ + (offset + idx) * PAGE_SIZE, frame_list_[offset + idx], vm::USER, vm::MAP_COPY_ON_WRITE);
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

private:
  // The frames.
  typedef std::vector<frame_t> frame_list_type;
  frame_list_type frame_list_;

  // Synchronize part of a buffer making it copy-on-write.
  void
  sync (size_t offset,
	size_t length)
  {
    if (begin_ != 0) {
      for (size_t idx = offset / PAGE_SIZE; idx != (offset + length) / PAGE_SIZE; ++idx) {
	frame_t actual = vm::logical_address_to_frame (begin_ + idx * PAGE_SIZE);
	if (frame_list_[idx] != actual) {
	  frame_manager::decref (frame_list_[idx]);
	  frame_list_[idx] = actual;
	  vm::remap (begin_ + idx * PAGE_SIZE, vm::USER, vm::MAP_COPY_ON_WRITE);
	}
      }
    }
  }

};

#endif /* __buffer_hpp__ */
