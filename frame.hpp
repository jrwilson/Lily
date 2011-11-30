#ifndef __frame_hpp__
#define __frame_hpp__

/*
  File
  ----
  frame.hpp
  
  Description
  -----------
  A frame is a page-sized unit of physical memory.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"

const int FRAME_SHIFT = 12;

struct page_directory_entry;
struct page_table_entry;

class frame
{
private:
  size_t f_;

public:
  frame () :
    f_ (0) { }

  explicit frame (const physical_address& address) :
    f_ (address.value () >> FRAME_SHIFT)
  { }

  explicit frame (const page_directory_entry& entry);

  explicit frame (const page_table_entry& entry);

  bool operator== (const frame& other) const {
    return f_ == other.f_;
  }

  bool operator< (const frame& other) const {
    return f_ < other.f_;
  }

  size_t operator- (const frame& other) const {
    return f_ - other.f_;
  }

  frame& operator+= (const size_t& offset) {
    f_ += offset;
    return *this;
  }

  operator physical_address () const {
    return physical_address (f_ << FRAME_SHIFT);
  }

  friend class page_table_entry;
  friend class page_directory_entry;
};

#endif /* __frame_hpp__ */
