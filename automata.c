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
#include "mm.h"
#include "scheduler.h"
#include "gdt.h"

typedef struct output_action output_action_t;
struct output_action {
  aid_t aid;
  unsigned int action_entry_point;
  unsigned int parameter;
};

struct automaton {
  privilege_t privilege;
  hash_map_t* actions;
  void* scheduler_context;
};
typedef struct automaton automaton_t;

static hash_map_t* automata = 0;
static aid_t next_aid;
static hash_map_t* bindings = 0;

static unsigned int
aid_hash_func (const void* x)
{
  return (unsigned int)x;
}

static int
aid_compare_func (const void* x,
		  const void* y)
{
  return x - y;
}

static unsigned int
action_entry_point_hash_func (const void* x)
{
  return (unsigned int)x;
}

static int
action_entry_point_compare_func (const void* x,
				 const void* y)
{
  return x - y;
}

static unsigned int
output_action_hash_func (const void* x)
{
  const output_action_t* ptr = x;
  return ptr->aid ^ ptr->action_entry_point ^ ptr->parameter;
}

static int
output_action_compare_func (const void* x,
			    const void* y)
{
  const output_action_t* p1 = x;
  const output_action_t* p2 = y;
  if (p1->aid != p2->aid) {
    return p1->aid - p2->aid;
  }
  else if (p1->action_entry_point != p2->action_entry_point) {
    return p1->action_entry_point - p2->action_entry_point;
  }
  else {
    return p1->parameter - p2->parameter;
  }
}

static automaton_t*
allocate_automaton (privilege_t privilege,
		    void* scheduler_context)
{
  automaton_t* ptr = kmalloc (sizeof (automaton_t));
  ptr->privilege = privilege;
  ptr->actions = allocate_hash_map (action_entry_point_hash_func, action_entry_point_compare_func);
  ptr->scheduler_context = scheduler_context;
  return ptr;
}

static void
automaton_set_action_type (automaton_t* ptr,
			   unsigned int action_entry_point,
			   action_type_t action_type)
{
  kassert (ptr != 0);
  kassert (!hash_map_contains (ptr->actions, (const void*)action_entry_point));
  hash_map_insert (ptr->actions, (const void*)action_entry_point, (void *)action_type);
}

static action_type_t
automaton_get_action_type (automaton_t* ptr,
			   unsigned int action_entry_point)
{
  kassert (ptr != 0);
  if (hash_map_contains (ptr->actions, (const void*)action_entry_point)) {
    action_type_t retval = (action_type_t)hash_map_find (ptr->actions, (const void*)action_entry_point);
    return retval;
  }
  else {
    return NO_ACTION;
  }
}

void
initialize_automata ()
{
  kassert (automata == 0);
  automata = allocate_hash_map (aid_hash_func, aid_compare_func);
  next_aid = 0;
  bindings = allocate_hash_map (output_action_hash_func, output_action_compare_func);
}

aid_t
create (privilege_t privilege)
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
set_action_type (aid_t aid,
		 unsigned int action_entry_point,
		 action_type_t action_type)
{
  kassert (automata != 0);
  automaton_t* ptr = hash_map_find (automata, (const void*)aid);
  kassert (ptr != 0);
  automaton_set_action_type (ptr, action_entry_point, action_type);
}

action_type_t
get_action_type (aid_t aid,
		 unsigned int action_entry_point)
{
  kassert (automata != 0);
  automaton_t* ptr = hash_map_find (automata, (const void*)aid);
  kassert (ptr != 0);
  return automaton_get_action_type (ptr, action_entry_point);
}

void
switch_to_automaton (aid_t aid,
		     unsigned int action_entry_point,
		     unsigned int parameter,
		     unsigned int input_value)
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
		"pop %%eax\n"
		"or $0x200, %%eax\n" /* Enable interupts on return. */
		"push %%eax\n"
		"pushl %2\n"
		"pushl %3\n"
		"movl %4, %%ecx\n"
		"movl %5, %%edx\n"
		"iret\n" :: "r"(stack_segment), "r"(USER_STACK_LIMIT), "m"(code_segment), "m"(action_entry_point), "m"(parameter), "m"(input_value));
}

static output_action_t*
allocate_output_action (aid_t aid,
			unsigned int action_entry_point,
			unsigned int parameter)
{
  output_action_t* ptr = kmalloc (sizeof (output_action_t));
  ptr->aid = aid;
  ptr->action_entry_point = action_entry_point;
  ptr->parameter = parameter;
  return ptr;
}

static input_action_t*
allocate_input_action (aid_t aid,
		       unsigned int action_entry_point,
		       unsigned int parameter)
{
  input_action_t* ptr = kmalloc (sizeof (input_action_t));
  ptr->aid = aid;
  ptr->action_entry_point = action_entry_point;
  ptr->parameter = parameter;
  ptr->next = 0;
  return ptr;
}

void
bind (aid_t output_aid,
      unsigned int output_action_entry_point,
      unsigned int output_parameter,
      aid_t input_aid,
      unsigned int input_action_entry_point,
      unsigned int input_parameter)
{
  kassert (bindings != 0);
  /* TODO:  All of the bind checks. */

  output_action_t output_action;
  output_action.aid = output_aid;
  output_action.action_entry_point = output_action_entry_point;
  output_action.parameter = output_parameter;

  if (!hash_map_contains (bindings, &output_action)) {
    hash_map_insert (bindings, allocate_output_action (output_aid, output_action_entry_point, output_parameter), 0);
  }

  input_action_t* input_action = allocate_input_action (input_aid, input_action_entry_point, input_parameter);
  input_action->next = hash_map_find (bindings, &output_action);
  hash_map_erase (bindings, &output_action);
  hash_map_insert (bindings, &output_action, input_action);
}

input_action_t*
get_bound_inputs (aid_t output_aid,
		  unsigned int output_action_entry_point,
		  unsigned int output_parameter)
{
  kassert (bindings != 0);

  output_action_t output_action;
  output_action.aid = output_aid;
  output_action.action_entry_point = output_action_entry_point;
  output_action.parameter = output_parameter;
  return hash_map_find (bindings, &output_action);
}
