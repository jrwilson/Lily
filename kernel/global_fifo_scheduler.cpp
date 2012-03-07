#include "global_fifo_scheduler.hpp"

global_fifo_scheduler::context_map_type global_fifo_scheduler::context_map_;
global_fifo_scheduler::queue_type global_fifo_scheduler::ready_queue_;

automaton* global_fifo_scheduler::current_automaton_;
caction global_fifo_scheduler::action_;
global_fifo_scheduler::input_action_list_type global_fifo_scheduler::input_action_list_;
global_fifo_scheduler::input_action_list_type::const_iterator global_fifo_scheduler::input_action_pos_;
buffer* global_fifo_scheduler::output_buffer_a_;
buffer* global_fifo_scheduler::output_buffer_b_;
