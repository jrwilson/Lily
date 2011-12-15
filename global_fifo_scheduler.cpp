#include "global_fifo_scheduler.hpp"

void
global_fifo_scheduler::add_automaton (automaton* automaton)
{
  // Allocate a new context and insert it into the map.
  // Inserting should succeed.
  automaton_context* c = new (system_alloc ()) automaton_context ();
  std::pair<context_map_type::iterator, bool> r = context_map_.insert (std::make_pair (automaton, c));
  kassert (r.second);
}

global_fifo_scheduler::execution_context global_fifo_scheduler::exec_context_;
global_fifo_scheduler::queue_type global_fifo_scheduler::ready_queue_;
global_fifo_scheduler::context_map_type global_fifo_scheduler::context_map_;
