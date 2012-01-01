#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

/*
  File
  ----
  system_automaton.hpp
  
  Description
  -----------
  Declarations for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include "action_traits.hpp"
#include "vm_def.hpp"
#include <unordered_map>
#include "system_allocator.hpp"

class automaton;
class buffer;

namespace system_automaton {
  typedef std::unordered_map<aid_t, automaton*, std::hash<aid_t>, std::equal_to<aid_t>, system_allocator<std::pair<const aid_t, automaton*> > > aid_map_type;

  class const_automaton_iterator : public std::iterator<std::forward_iterator_tag, const automaton> {
  public:

    const_automaton_iterator (aid_map_type::const_iterator p) :
      p_ (p)
    { }

    inline bool
    operator== (const const_automaton_iterator& other) const
    {
      return p_ == other.p_;
    }

    inline const_automaton_iterator&
    operator++ ()
    {
      ++p_;
      return *this;
    }

    inline const automaton*
    operator-> ()
    {
      return p_->second;
    }

  private:
    aid_map_type::const_iterator p_;
  };

  const_automaton_iterator
  automaton_begin ();

  const_automaton_iterator
  automaton_end ();

  void
  create_system_automaton (buffer* automaton_buffer,
			   size_t automaton_size,
			   buffer* data_buffer,
			   size_t data_size);
}

#endif /* __system_automaton_hpp__ */
