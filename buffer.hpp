#ifndef __buffer_hpp__
#define __buffer_hpp__

#include "vm_area.hpp"

// TODO:  The zero frame might be over-referenced.  Consider allocating a new one to keep the reference count in range.

class buffer : public vm_area_base {
public:
  buffer (size_t size) :
    vm_area_base (0, 0),
    frame_list_ (size / PAGE_SIZE, vm::page_table_entry (vm::zero_frame (), vm::USER, vm::NOT_WRITABLE, vm::NOT_PRESENT))
  {
    frame_manager::incref (vm::zero_frame (), frame_list_.size ());
  }
  
  buffer (buffer& other,
	  size_t offset,
	  size_t length) :
    vm_area_base (0, 0),
    frame_list_ (length / PAGE_SIZE, vm::page_table_entry (vm::zero_frame (), vm::USER, vm::NOT_WRITABLE, vm::NOT_PRESENT))
  {
    frame_manager::incref (vm::zero_frame (), frame_list_.size ());
    assign_ (0, frame_list_.size (), other, offset / PAGE_SIZE);
  }

  buffer (const buffer& other) :
    vm_area_base (0, 0),
    frame_list_ (other.frame_list_)
  {
    // This is used when copying between automata.
    // The source must be unmapped, i.e., not present and not writable.
    for (frame_list_type::const_iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      kassert (pos->present_ == vm::NOT_PRESENT);
      kassert (pos->writable_ == vm::NOT_WRITABLE);
      frame_manager::incref (pos->frame_);
    }
  }

  buffer (frame_t begin,
  	  frame_t end) :
    vm_area_base (0, 0),
    frame_list_ (end - begin, vm::page_table_entry (0, vm::USER, vm::NOT_WRITABLE, vm::NOT_PRESENT))
  {
    for (frame_list_type::iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      // Note:  Not incrementing the reference count of the frame.
      pos->frame_ = begin++;
    }    
  }

  ~buffer ()
  {
    // Free the frames.
    for (frame_list_type::const_iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      frame_manager::decref (pos->frame_);
    }
  }

  void
  set_end (logical_address_t end)
  {
    end_ = end;
    begin_ = end - size ();
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
    frame_list_.resize (begin + size / PAGE_SIZE, vm::page_table_entry (vm::zero_frame (), vm::USER, vm::NOT_WRITABLE, vm::NOT_PRESENT));
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
    size_t retval = this->size ();
    size_t begin = frame_list_.size ();
    frame_list_.resize (begin + length / PAGE_SIZE, vm::page_table_entry (vm::zero_frame (), vm::USER, vm::NOT_WRITABLE, vm::NOT_PRESENT));
    frame_manager::incref (vm::zero_frame (), frame_list_.size () - begin);
    assign_ (begin, frame_list_.size (), other, offset / PAGE_SIZE);
    return retval;
  }

  void
  assign (size_t offset,
	  buffer& src,
	  size_t src_offset,
	  size_t length)
  {
    assign_ (offset / PAGE_SIZE, (offset + length) / PAGE_SIZE, src, src_offset / PAGE_SIZE);
  }

  virtual void
  page_fault (logical_address_t address,
	      page_fault_error_t error,
	      volatile registers*)
  {
    kassert (data_context (error));

    size_t idx = physical_address_to_frame (address - begin_);

    // The frame in question should always be copy on write.
    kassert (frame_list_[idx].writable_ == vm::NOT_WRITABLE);

    if (not_present (error)) {
      if (read_context (error)) {
	// The page is not present and we are trying to read.
	// Map.
	vm::map (address, frame_list_[idx].frame_, vm::USER, vm::NOT_WRITABLE);
	frame_list_[idx].present_ = vm::PRESENT;
      }
      else {
	// The page is not present and we are trying to write.
	// Copy.
	// Setup destination and source frames.
	frame_t dst = frame_manager::alloc ();
	frame_t src = frame_list_[idx].frame_;
	// Map the src to the stub.
	vm::map (vm::get_stub1 (), src, vm::USER, vm::NOT_WRITABLE);
	// Map the dst to the address.
	vm::map (address, dst, vm::USER, vm::WRITABLE);
	// Copy.
	memcpy (reinterpret_cast<void*> (align_down (address, PAGE_SIZE)), reinterpret_cast<const void*> (vm::get_stub1 ()), PAGE_SIZE);
	// Unmap the src and decrement its reference count.
	vm::unmap (vm::get_stub1 ());
	frame_manager::decref (src);
	// Record the dst.
	frame_list_[idx].frame_ = dst;
	frame_list_[idx].writable_ = vm::WRITABLE;
	frame_list_[idx].present_ = vm::PRESENT;
      }
    }
    else {
      kassert (write_context (error));
      kassert (frame_list_[idx].present_ == vm::PRESENT);
      // The user tried to write to page mapped previously as read-only.
      // Copy.
      // Exactly the same as previous copy except...
      frame_t dst = frame_manager::alloc ();
      frame_t src = frame_list_[idx].frame_;
      vm::map (vm::get_stub1 (), src, vm::USER, vm::NOT_WRITABLE);
      // we need to unmap the current frame to make room.
      vm::unmap (address);
      vm::map (address, dst, vm::USER, vm::WRITABLE);
      memcpy (reinterpret_cast<void*> (align_down (address, PAGE_SIZE)), reinterpret_cast<const void*> (vm::get_stub1 ()), PAGE_SIZE);
      vm::unmap (vm::get_stub1 ());
      frame_manager::decref (src);
      frame_list_[idx].frame_ = dst;
      frame_list_[idx].writable_ = vm::WRITABLE;
    }
  }

  void
  unmap ()
  {
    if (begin_ != 0) {
      for (size_t idx = 0; idx < frame_list_.size (); ++idx) {
	if (frame_list_[idx].present_ == vm::PRESENT) {
	  vm::unmap (begin_ + idx * PAGE_SIZE);
	  frame_list_[idx].present_ = vm::NOT_PRESENT;
	  // Convert to copy-on-write as well.
	  frame_list_[idx].writable_ = vm::NOT_WRITABLE;
	}
      }

      begin_ = 0;
      end_ = 0;
    }
  }

private:
  // The frames.
  typedef std::vector<vm::page_table_entry, system_allocator<vm::page_table_entry> > frame_list_type;
  frame_list_type frame_list_;

  // Assigns and (1) unmaps/decrefs overwritten frames and (2) makes the source buffer copy-on-write.
  void
  assign_ (size_t begin,
	   size_t end,
	   buffer& src,
	   size_t src_begin)
  {
    for (; begin != end; ++begin, ++src_begin) {
      // Make the source copy on write.
      if (src.frame_list_[src_begin].writable_ == vm::WRITABLE) {
	// Should be present.
	kassert (src.frame_list_[src_begin].present_ == vm::PRESENT);
	// Remap copy-on-write.
	vm::remap (src.begin_ + PAGE_SIZE * (src_begin), vm::USER, vm::NOT_WRITABLE);
	src.frame_list_[src_begin].writable_ = vm::NOT_WRITABLE;
      }

      // Dispense with old frame.
      if (frame_list_[begin].present_ == vm::PRESENT) {
	// Unmap.
	vm::unmap (begin_ + PAGE_SIZE * begin);
      }
      frame_manager::decref (frame_list_[begin].frame_);

      // Copy.
      frame_list_[begin].frame_ = src.frame_list_[src_begin].frame_;
      frame_manager::incref (frame_list_[begin].frame_);
    }
  }

};

#endif /* __buffer_hpp__ */
