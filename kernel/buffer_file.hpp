#ifndef __buffer_file_hpp__
#define __buffer_file_hpp__

class buffer_file {
private:
  const logical_address_t begin_;
  const logical_address_t end_;
  size_t offset_;

public:
  buffer_file (logical_address_t begin,
	       logical_address_t end) :
    begin_ (begin),
    end_ (end),
    offset_ (0)
  { }

  const void*
  readp (size_t size)
  {
    size_t remaining = end_ - (begin_ + offset_);
    if (size <= remaining) {
      const void* retval = reinterpret_cast<const void*> (begin_ + offset_);
      offset_ += size;
      return retval;
    }
    else {
      return 0;
    }
  }

  void
  seek (size_t offset) {
    // Can't seek past end.
    offset_ = min (offset, (size_t)(end_ - begin_));
  }

  size_t
  position (void) const
  {
    return offset_;
  }
};

#endif /* __buffer_file_hpp__ */
