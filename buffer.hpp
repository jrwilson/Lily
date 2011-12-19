#ifndef __buffer_hpp__
#define __buffer_hpp__

class buffer {
public:
  enum status_t {
    OPEN,
    CLOSED,
  };
  
  buffer (size_t size) :
    status_ (OPEN),
    reference_count_ (1)
  {
    // Allocate frames.
    for (size = align_up (size, PAGE_SIZE);
	 size != 0;
	 size -= PAGE_SIZE) {
      frame_list_.push_back (vm::page_table_entry (0, vm::USER, vm::WRITABLE, vm::NOT_PRESENT));
    }
  }

  ~buffer ()
  {
    // Free the frames.
    for (frame_list_type::const_iterator pos = frame_list_.begin ();
	 pos != frame_list_.end ();
	 ++pos) {
      if (pos->present_ == vm::PRESENT) {
	frame_manager::decref (pos->frame_);
      }
    }
  }

  size_t
  size () const
  {
    return frame_list_.size () * PAGE_SIZE;
  }
  
  size_t
  incref ()
  {
    // Changing the reference count always closes a buffer.
    close ();
    return ++reference_count_;
  }

  size_t
  decref ()
  {
    // Changing the reference count always closes a buffer.
    close ();
    return --reference_count_;
  }

  void
  page_fault (const void* base,
	      const void* address,
	      page_fault_error_t error)
  {
    size_t idx = physical_address_to_frame (static_cast<const uint8_t*> (address) - static_cast<const uint8_t*> (base));

    kassert (not_present (error));
    kassert (frame_list_[idx].present_ == vm::NOT_PRESENT);
    kassert (data_context (error));

    if (read_context (error)) {
      kassert (0);
    }
    else {
      if (frame_list_[idx].writable_ == vm::WRITABLE) {
	frame_list_[idx].frame_ = frame_manager::alloc ();
	frame_list_[idx].present_ = vm::PRESENT;

	vm::map (address, frame_list_[idx].frame_, vm::USER, vm::WRITABLE);
	// Clear the frame.
	memset (const_cast<void*> (align_down (address, PAGE_SIZE)), 0x00, PAGE_SIZE);
      }
      else {
	// TODO:  Automaton tried to write to a read-only area.
	kassert (0);
      }
    }
  }

private:
  // OPEN means the set of frames can be altered.  CLOSED means it cannot.
  status_t status_;
  // The number of references.
  size_t reference_count_;
  // The frames.
  typedef std::vector<vm::page_table_entry, system_allocator<vm::page_table_entry> > frame_list_type;
  frame_list_type frame_list_;

  void
  close ()
  {
    if (status_ != CLOSED) {
      status_ = CLOSED;
      // Change all of the permissions to read only.
      // Free the frames.
      for (frame_list_type::iterator pos = frame_list_.begin ();
	   pos != frame_list_.end ();
	   ++pos) {
	pos->writable_ = vm::NOT_WRITABLE;
      }
    }
  }

};

#endif /* __buffer_hpp__ */
