#ifndef __buffer_hpp__
#define __buffer_hpp__

#include "vm_area.hpp"

extern int zero_page;

class buffer : public vm_area_base {
public:
  buffer (size_t size) :
    vm_area_base (0, 0)
  {
    for (size = align_up (size, PAGE_SIZE);
	 size != 0;
	 size -= PAGE_SIZE) {
      // Copy-on-write the zero frame.
      frame_manager::incref (physical_address_to_frame (reinterpret_cast<physical_address_t> (&zero_page)));
      frame_list_.push_back (vm::page_table_entry (physical_address_to_frame (reinterpret_cast<physical_address_t> (&zero_page)), vm::USER, vm::NOT_WRITABLE, vm::PRESENT));
    }
  }

  buffer (const buffer& other) :
    vm_area_base (0, 0),
    frame_list_ (other.frame_list_)
  {
    for (frame_list_type::iterator pos = frame_list_.begin (); pos != frame_list_.end (); ++pos) {
      kassert (pos->writable_ == vm::NOT_WRITABLE);
      frame_manager::incref (pos->frame_);
    }
  }

  ~buffer ()
  {
    // Free the frames.
    for (frame_list_type::const_iterator pos = frame_list_.begin ();
	 pos != frame_list_.end ();
	 ++pos) {
      frame_manager::decref (pos->frame_);
    }
  }

  void
  set_end (const void* end)
  {
    end_ = end;
    begin_ = static_cast<const uint8_t*> (end) - size ();
  }

  size_t
  size () const
  {
    return frame_list_.size () * PAGE_SIZE;
  }

  virtual void
  page_fault (const void* address,
	      page_fault_error_t error,
	      volatile registers*)
  {
    kassert (data_context (error));

    size_t idx = physical_address_to_frame (static_cast<const uint8_t*> (address) - static_cast<const uint8_t*> (begin_));

    if (not_present (error)) {
      // The source should be copy on write.
      kassert (frame_list_[idx].writable_ == vm::NOT_WRITABLE);

      if (read_context (error)) {
	// The page is not present and we are trying to read.
	// Map.
	vm::map (address, frame_list_[idx].frame_, vm::USER, vm::NOT_WRITABLE);
	kout << "Mapping " << hexformat (address) << " not writable" << endl;
      }
      else {
	// The page is not present and we are trying to write.
	// Copy.
	// Setup destination and source frames.
	frame_t dst = frame_manager::alloc ();
	frame_t src = frame_list_[idx].frame_;
	// Map the dst on the address.
	vm::map (address, dst, vm::USER, vm::WRITABLE);
	kout << "Mapping " << hexformat (address) << " writable" << endl;
	// Map the src on the stub.
	vm::map (vm::get_stub (), src, vm::USER, vm::NOT_WRITABLE);
	// Copy.
	memcpy (const_cast<void*> (align_down (address, PAGE_SIZE)), vm::get_stub (), PAGE_SIZE);
	// Unmap the src and decrement its reference count.
	vm::unmap (vm::get_stub ());
	frame_manager::decref (src);
	// Record the dst.
	frame_list_[idx].frame_ = dst;
	frame_list_[idx].writable_ = vm::WRITABLE;
      }
    }
    else {
      // The user tried to write to page mapped read-only.
      kassert (write_context (error));
      
      // TODO
      kassert (0);
    }
  }

private:
  // The frames.
  typedef std::vector<vm::page_table_entry, system_allocator<vm::page_table_entry> > frame_list_type;
  frame_list_type frame_list_;
};

#endif /* __buffer_hpp__ */
