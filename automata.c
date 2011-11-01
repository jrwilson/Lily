/*
  File
  ----
  automata.c
  
  Description
  -----------
  Functions for managing the set of automata.

  Authors:
  Justin R. Wilson
*/

#include "automata.h"
#include "hash_map.h"
#include "kassert.h"
#include "memory.h"
#include "scheduler.h"

struct automaton {
  privilege_t privilege;
  void* scheduler_context;
};
typedef struct automaton automaton_t;

static hash_map_t* automata = 0;
static aid_t next_aid;

static unsigned int
hash_func (const void* x)
{
  return (unsigned int)x;
}

static int
compare_func (const void* x,
	      const void* y)
{
  return x - y;
}

static automaton_t*
allocate_automaton (privilege_t privilege,
		    void* scheduler_context)
{
  automaton_t* ptr = kmalloc (sizeof (automaton_t));
  ptr->privilege = privilege;
  ptr->scheduler_context = scheduler_context;
  return ptr;
}

void
initialize_automata ()
{
  kassert (automata == 0);
  automata = allocate_hash_map (hash_func, compare_func);
  next_aid = 0;
}

aid_t
create_automaton (privilege_t privilege)
{
  kassert (automata != 0);
  /* Make sure that an aid is available.  We'll probably run out of memory before this happens ;) */
  kassert (hash_map_size (automata) !=  2147483648U);

  while (hash_map_contains (automata, (const void*)next_aid)) {
    ++next_aid;
    if (next_aid < 0) {
      next_aid = 0;
    }
  }

  hash_map_insert (automata, (const void*)next_aid, allocate_automaton (privilege, allocate_scheduler_context (next_aid)));

  return next_aid;
}

void*
get_scheduler_context (aid_t aid)
{
  kassert (automata != 0);
  automaton_t* ptr = hash_map_find (automata, (const void*)aid);

  if (ptr != 0) {
    return ptr->scheduler_context;
  }
  else {
    return 0;
  }  
}

void
switch_to_automaton (aid_t aid,
		     unsigned int action_entry_point,
		     unsigned int parameter)
{
  kassert (automata != 0);
  automaton_t* ptr = hash_map_find (automata, (const void*)aid);
  kassert (ptr != 0);

  unsigned int stack_segment;
  unsigned int code_segment;

  switch (ptr->privilege) {
  case RING0:
    stack_segment = KERNEL_DATA_SELECTOR | RING0;
    code_segment = KERNEL_CODE_SELECTOR | RING0;
    break;
  case RING1:
  case RING2:
    /* These rings are not supported. */
    kassert (0);
    break;
  case RING3:
    /* Not supported yet. */
    kassert (0);
    break;
  }

  asm volatile ("mov %0, %%eax\n"
		"mov %%ax, %%ss\n"
		"mov %1, %%eax\n"
		"mov %%eax, %%esp\n"
		"pushf\n"
		"pushl %2\n"
		"pushl %3\n"
		"movl %4, %%eax\n"
		"iret\n" :: "r"(stack_segment), "r"(USER_STACK_LIMIT), "m"(code_segment), "m"(action_entry_point), "m"(parameter) : "eax");
}
