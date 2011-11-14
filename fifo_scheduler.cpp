/*
  File
  ----
  fifo_scheduler.h
  
  Description
  -----------
  A fifo scheduler for automata.

  Authors:
  Justin R. Wilson
*/

#include "fifo_scheduler.hpp"
#include "kassert.hpp"
#include "syscall.hpp"

struct fifo_scheduler::entry {
  entry* next;
  void* action_entry_point;
  parameter_t parameter;
};

fifo_scheduler::fifo_scheduler (list_allocator& allocator) :
  allocator_ (allocator),
  ready_ (0),
  free_ (0),
  ready_list_ (list_allocator_wrapper<entry> (allocator))
{ }

fifo_scheduler::entry*
fifo_scheduler::allocate_scheduler_entry (void* action_entry_point,
					  parameter_t parameter)
{
  entry* p;
  if (free_ != 0) {
    p = free_;
    free_ = p->next;
  }
  else {
    p = static_cast<entry*> (allocator_.alloc (sizeof (entry)));
  }
  p->next = 0;
  p->action_entry_point = action_entry_point;
  p->parameter = parameter;
  return p;
}

void
fifo_scheduler::add (void* action_entry_point,
		     parameter_t parameter)
{
  entry** p;
  for (p = &ready_; (*p) != 0 && !((*p)->action_entry_point == action_entry_point && (*p)->parameter == parameter); p = &((*p)->next)) ;;
  if (*p == 0) {
    *p = allocate_scheduler_entry (action_entry_point, parameter);
  }
}

void
fifo_scheduler::free_scheduler_entry (entry* p)
{
  p->next = free_;
  free_ = p;
}

void
fifo_scheduler::remove (void* action_entry_point,
			parameter_t parameter)
{
  entry** p;
  for (p = &ready_; (*p) != 0 && !((*p)->action_entry_point == action_entry_point && (*p)->parameter == parameter); p = &((*p)->next)) ;;
  if (*p != 0) {
    entry* tmp = *p;
    *p = tmp->next;
    free_scheduler_entry (tmp);
  }
}

void
fifo_scheduler::finish (bool output_status,
			value_t output_value)
{
  if (ready_ != 0) {
    /* Schedule. */
    sys_schedule (ready_->action_entry_point, ready_->parameter, output_status, output_value);
  }
  else {
    /* Don't schedule. */
    sys_finish (output_status, output_value);
  }
}
