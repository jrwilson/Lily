#ifndef __aid_manager_hpp__
#define __aid_manager_hpp__

/*
  File
  ----
  aid_manager.hpp
  
  Description
  -----------
  A class for managing the set of automaton identifiers (aids).

  Authors:
  Justin R. Wilson
*/

#include <unordered_set>
#include <algorithm>
#include "aid.hpp"
#include "system_allocator.hpp"

class aid_manager {
public:
  static inline aid_t
  alloc ()
  {
    while (aid_set_.find (current_) != aid_set_.end ()) {
      current_ = std::max (current_ + 1, 0);
    }
    aid_set_.insert (current_);
    return current_++;
  }

  static inline void
  free (aid_t aid)
  {
    aid_set_.erase (aid);
  }

private:
  static aid_t current_;
  typedef std::unordered_set<aid_t, std::hash<aid_t>, std::equal_to<aid_t>, system_allocator<aid_t> > aid_set_type;
  static aid_set_type aid_set_;
};

#endif /* __aid_manager_hpp__ */
