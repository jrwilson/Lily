/*
  File
  ----
  system_automaton.cpp
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.hpp"
#include "system_allocator.hpp"
#include "action_macros.hpp"

template <>
automaton_interface* system_automaton<system_allocator_tag, system_allocator>::instance_ = 0;

template <>
fifo_scheduler<system_allocator_tag, system_allocator>* system_automaton<system_allocator_tag, system_allocator>::scheduler_ = 0;

template <>
bool system_automaton<system_allocator_tag, system_allocator>::flag_ = true;

#define COMMA ,

UP_INTERNAL (system_automaton<system_allocator_tag COMMA system_allocator>, sa_first);
// UV_P_OUTPUT (system_automaton_init, automaton*, *sched);
// UV_UP_INPUT (system_automaton_create_request, *sched);
// UV_UP_OUTPUT (system_automaton_create_response, *sched);
// UV_UP_INPUT (system_automaton_bind_request, *sched);
// UV_UP_OUTPUT (system_automaton_bind_response, *sched);
// UV_UP_INPUT (system_automaton_unbind_request, *sched);
// UV_UP_OUTPUT (system_automaton_unbind_response, *sched);
// UV_UP_INPUT (system_automaton_destroy_request, *sched);
// UV_UP_OUTPUT (system_automaton_destroy_response, *sched);
