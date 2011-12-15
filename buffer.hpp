#ifndef __buffer_hpp__
#define __buffer_hpp__

class automaton;

static const int MAX_REFERENCE_COUNT = 512;

class buffer {
public:
  buffer (size_t size,
	  automaton* creator) :
    size_ (size)
  {
    std::pair<reference_map_type::iterator, bool> r = reference_map_.insert (std::make_pair (creator, 0));
    ++r.first->second;
  }

  size_t
  size (automaton* a) const
  {
    if (reference_map_.find (a) != reference_map_.end ()) {
      // Automaton has a reference.
      return size_;
    }
    // Automaton does not have a reference.  Error.
    return -1;
  }

  int
  incref (automaton* a)
  {
    reference_map_type::iterator pos = reference_map_.find (a);
    if (pos != reference_map_.end ()) {
      // Automaton has a reference.
      if (pos->second < MAX_REFERENCE_COUNT) {
	++pos->second;
      }
      return pos->second;
    }
    // Automaton does not have a reference.
    return -1;
  }

  int
  decref (automaton* a)
  {
    reference_map_type::iterator pos = reference_map_.find (a);
    if (pos != reference_map_.end ()) {
      // Automaton has a reference.
      return --pos->second;
    }
    // Automaton does not have a reference.
    return -1;
  }
  
  bool
  empty () const
  {
    return reference_map_.empty ();
  }

private:
  // The size of the buffer.
  size_t size_;
  // Reference counts.
  typedef std::unordered_map<automaton*, int, std::hash<automaton*>, std::equal_to<automaton*>, system_allocator<std::pair<automaton* const, int> > > reference_map_type;
  reference_map_type reference_map_;
  // // Child buffers.
  // typedef std::unordered_set<buffer*, std::hash<buffer*>, std::equal_to<buffer*>, system_allocator<buffer*> > children_set_type;
  // children_set_type children_set_;
};

#endif /* __buffer_hpp__ */
