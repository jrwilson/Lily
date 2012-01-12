#ifndef __system_automaton_private_hpp__
#define __system_automaton_private_hpp__

/*
  File
  ----
  system_automaton_private.hpp
  
  Description
  -----------
  Declarations for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include <system_automaton.hpp>

extern "C" void first (no_param_t);
extern "C" void create_request (aid_t, void*, size_t, bid_t, size_t);
extern "C" void create (no_param_t);
extern "C" void init (aid_t);
extern "C" void create_response (aid_t);
extern "C" void bind_request (aid_t, void*, size_t, bid_t, size_t);
extern "C" void bind (no_param_t);
extern "C" void bind_response (aid_t);
extern "C" void loose_request (aid_t, void*, size_t, bid_t, size_t);
extern "C" void loose (no_param_t);
extern "C" void loose_response (aid_t);
extern "C" void destroy_request (aid_t, void*, size_t, bid_t, size_t);
extern "C" void destroy (no_param_t);
extern "C" void destroy_response (aid_t);

namespace system_automaton {

  typedef action_traits<internal_action> first_traits;
#define SA_FIRST_TRAITS system_automaton::first_traits
#define SA_FIRST_NAME "first"
#define SA_FIRST_DESCRIPTION ""
#define SA_FIRST_COMPARE M_NO_COMPARE
#define SA_FIRST_ACTION M_INTERNAL
#define SA_FIRST_PARAMETER M_NO_PARAMETER

  typedef action_traits<internal_action> create_traits;
#define SA_CREATE_TRAITS system_automaton::create_traits
#define SA_CREATE_NAME "create"
#define SA_CREATE_DESCRIPTION ""
#define SA_CREATE_COMPARE M_NO_COMPARE
#define SA_CREATE_ACTION M_INTERNAL
#define SA_CREATE_PARAMETER M_NO_PARAMETER

  typedef action_traits<internal_action> bind_traits;
#define SA_BIND_TRAITS system_automaton::bind_traits
#define SA_BIND_NAME "bind"
#define SA_BIND_DESCRIPTION ""
#define SA_BIND_COMPARE M_NO_COMPARE
#define SA_BIND_ACTION M_INTERNAL
#define SA_BIND_PARAMETER M_NO_PARAMETER

  typedef action_traits<internal_action> loose_traits;
#define SA_LOOSE_TRAITS system_automaton::loose_traits
#define SA_LOOSE_NAME "loose"
#define SA_LOOSE_DESCRIPTION ""
#define SA_LOOSE_COMPARE M_NO_COMPARE
#define SA_LOOSE_ACTION M_INTERNAL
#define SA_LOOSE_PARAMETER M_NO_PARAMETER

  typedef action_traits<internal_action> destroy_traits;
#define SA_DESTROY_TRAITS system_automaton::destroy_traits
#define SA_DESTROY_NAME "destroy"
#define SA_DESTROY_DESCRIPTION ""
#define SA_DESTROY_COMPARE M_NO_COMPARE
#define SA_DESTROY_ACTION M_INTERNAL
#define SA_DESTROY_PARAMETER M_NO_PARAMETER

}

#endif /* __system_automaton_private_hpp__ */
