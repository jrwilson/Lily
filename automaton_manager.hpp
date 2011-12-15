#ifndef __automaton_manager_hpp__
#define __automaton_manager_hpp__

/*
  File
  ----
  automaton_manager.hpp
  
  Description
  -----------
  Manage automata.

  Authors:
  Justin R. Wilson
*/

#include "automaton.hpp"
#include "rts.hpp"

class automaton_manager {
public:

  static automaton*
  create (descriptor_constants::privilege_t privilege,
	  frame_t frame)
  {
    // Generate an id.
    aid_t aid = current_;
    while (aid_map_.find (aid) != aid_map_.end ()) {
      aid = std::max (aid + 1, 0); // Handles overflow.
    }
    current_ = aid + 1;

    // Create the automaton and insert it into the map.
    automaton* a = new (system_alloc ()) automaton (aid, privilege, frame);
    aid_map_.insert (std::make_pair (aid, a));

    // Add to the scheduler.
    rts::scheduler.add_automaton (a);

    return a;
  }

private:
  static aid_t current_;
  typedef std::unordered_map<aid_t, automaton*, std::hash<aid_t>, std::equal_to<aid_t>, system_allocator<std::pair<const aid_t, automaton*> > > aid_map_type;
  static aid_map_type aid_map_;
};

#endif /* __automaton_manager_hpp__ */
