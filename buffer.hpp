#ifndef __buffer_hpp__
#define __buffer_hpp__

struct buffer {
  enum status_t {
    OPEN,
    CLOSED,
  };

  buffer (bid_t b,
	  size_t size) :
    bid (b),
    status (OPEN),
    size (size),
    reference_count (0)
  {
    // We imply ourselves.
    implied_set.insert (this);
  }

  bid_t bid;
  status_t status;
  // The size of the buffer in bytes.
  size_t size;
  // The number of refernces.
  size_t reference_count;
  // Implied set of buffers.
  typedef std::unordered_set<buffer*, std::hash<buffer*>, std::equal_to<buffer*>, system_allocator<buffer*> > implied_set_type;
  implied_set_type implied_set;
};

#endif /* __buffer_hpp__ */
