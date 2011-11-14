#ifndef __fifo_scheduler_hpp__
#define __fifo_scheduler_hpp__

/*
  File
  ----
  fifo_scheduler.hpp
  
  Description
  -----------
  A fifo scheduler for automata.

  Authors:
  Justin R. Wilson
*/

#include "types.hpp"
#include "list_allocator.hpp"
#include "list.hpp"
#include "unordered_set.hpp"

template <class T>
class list_allocator_wrapper {
public:
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef T* pointer;
  typedef const T* const_pointer;

  typedef T& reference;
  typedef const T& const_reference;

  list_allocator_wrapper (list_allocator&) { }
};

class fifo_scheduler {
private:
  struct entry;

  list_allocator& allocator_;
  entry* ready_;
  entry* free_;

  list<entry, list_allocator_wrapper<entry> > ready_list_;
  //unordered_set<entry> ready_set_;

  entry*
  allocate_scheduler_entry (void* action_entry_point,
			    parameter_t parameter);

  void
  free_scheduler_entry (entry* p);

public:
  fifo_scheduler (list_allocator& allocator);

  void
  add (void* action_entry_point,
       parameter_t parameter);

  void
  remove (void* action_entry_point,
	  parameter_t parameter);

  void
  finish (bool output_status,
	  value_t output_value);
};

#endif /* __fifo_scheduler_hpp__ */
