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
  /* Can't map into this area. */
  void* memory_ceiling;
} automaton_t;

void
automaton_initialize (automaton_t* ptr,
		      privilege_t privilege,
		      size_t page_directory,
		      void* stack_pointer,
		      void* memory_ceiling);

automaton_t*
automaton_allocate (privilege_t privilege,
		    size_t page_directory,
		    void* stack_pointer,
		    void* memory_ceiling) __attribute__((warn_unused_result));

int
automaton_insert_vm_area (automaton_t* ptr,
			  const vm_area_t* area) __attribute__((warn_unused_result));

void*
automaton_alloc (automaton_t* automaton,
		 size_t size,
		 syserror_t* error) __attribute__((warn_unused_result));

#endif /* __automaton_h__ */

