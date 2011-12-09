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

template <class Alloc, template <typename> class Allocator>
class aid_manager {
private:
  aid_t current_;
  typedef std::unordered_set<aid_t, std::hash<aid_t>, std::equal_to<aid_t>, Allocator<aid_t> > aid_set_type;
  aid_set_type aid_set_;


public:
  aid_manager (Alloc& alloc) :
    current_ (0),
    aid_set_ (3, typename aid_set_type::hasher (), typename aid_set_type::key_equal (), typename aid_set_type::allocator_type (alloc))
  { }

  aid_t
  allocate ()
  {
    while (aid_set_.find (current_) != aid_set_.end ()) {
      current_ = std::max (current_ + 1, 0);
    }
    return current_++;
  }
};

#endif /* __aid_manager_hpp__ */
