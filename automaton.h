#ifndef __automaton_h__
#define __automaton_h__

/*
  File
  ----
  automaton.h
  
  Description
  -----------
  The automaton data structure.

  Authors:
  Justin R. Wilson
*/

#include "vm_manager.h"
#include "hash_map.h"
#include "syscall_def.h"
#include "vm_area.h"

typedef enum {
  NO_ACTION,
  INPUT,
  OUTPUT,
  INTERNAL
} action_type_t;

typedef struct scheduler_context scheduler_context_t;

typedef struct {
  /* Automata execute at a certain privilege level. */
  privilege_t privilege;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  hash_map_t* actions;
  /* The scheduler uses this object. */
  scheduler_context_t* scheduler_context;
  /* Physical address of the page directory. */
  size_t page_directory;
  /* Stack pointer (constant). */
  void* stack_pointer;
  /* Memory map. */
  vm_area_t* memory_map_begin;
  vm_area_t* memory_map_end;
  /* Can map between [floor, ceiling). */
  void* memory_floor;
  void* memory_ceiling;
  /* Default privilege for new VM_AREA_DATA. */
  page_privilege_t page_privilege;
} automaton_t;

void
automaton_initialize (automaton_t* ptr,
		      privilege_t privilege,
		      size_t page_directory,
		      void* stack_pointer,
		      void* memory_ceiling,
		      page_privilege_t page_privilege);

automaton_t*
automaton_allocate (privilege_t privilege,
		    size_t page_directory,
		    void* stack_pointer,
		    void* memory_ceiling,
		    page_privilege_t page_privilege) __attribute__((warn_unused_result));

int
automaton_insert_vm_area (automaton_t* ptr,
			  const vm_area_t* area) __attribute__((warn_unused_result));

void*
automaton_alloc (automaton_t* automaton,
		 size_t size,
		 syserror_t* error) __attribute__((warn_unused_result));

void
automaton_set_action_type (automaton_t* ptr,
			   void* action_entry_point,
			   action_type_t action_type);

action_type_t
automaton_get_action_type (automaton_t* automaton,
			   void* action_entry_point);

void
automaton_execute (automaton_t* ptr,
		   void* action_entry_point,
		   parameter_t parameter,
		   value_t input_value);

#endif /* __automaton_h__ */

