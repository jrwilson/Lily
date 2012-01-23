#include "global_fifo_scheduler.hpp"

global_fifo_scheduler::execution_context global_fifo_scheduler::exec_context_;
global_fifo_scheduler::queue_type global_fifo_scheduler::ready_queue_;
global_fifo_scheduler::context_map_type global_fifo_scheduler::context_map_;
