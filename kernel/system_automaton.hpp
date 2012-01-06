#ifndef __system_automaton_hpp__
#define __system_automaton_hpp__

/*
  File
  ----
  system_automaton.hpp
  
  Description
  -----------
  Declarations for the system automaton.

  Authors:
  Justin R. Wilson
*/

#include <action_traits.hpp>

typedef action_traits<internal_action> first_traits;
#define FIRST_TRAITS first_traits
#define FIRST_NAME "first"
#define FIRST_DESCRIPTION ""
#define FIRST_ACTION M_INTERNAL
#define FIRST_PARAMETER M_NO_PARAMETER
extern "C" void first (no_param_t);

typedef action_traits<input_action, auto_parameter> create_request_traits;
#define CREATE_REQUEST_TRAITS create_request_traits
#define CREATE_REQUEST_NAME "create_request"
#define CREATE_REQUEST_DESCRIPTION "TODO: create_request description"
#define CREATE_REQUEST_ACTION M_INPUT
#define CREATE_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void create_request (aid_t, void*, size_t, bid_t, size_t);

typedef action_traits<internal_action> create_traits;
#define CREATE_TRAITS create_traits
#define CREATE_NAME "create"
#define CREATE_DESCRIPTION ""
#define CREATE_ACTION M_INTERNAL
#define CREATE_PARAMETER M_NO_PARAMETER
extern "C" void create (no_param_t);

typedef action_traits<output_action, parameter<aid_t> > init_traits;
#define INIT_TRAITS init_traits
#define INIT_NAME "init"
#define INIT_DESCRIPTION "TODO: init description"
#define INIT_ACTION M_OUTPUT
#define INIT_PARAMETER M_PARAMETER
extern "C" void init (aid_t);

typedef action_traits<output_action, auto_parameter> create_response_traits;
#define CREATE_RESPONSE_TRAITS create_response_traits
#define CREATE_RESPONSE_NAME "create_response"
#define CREATE_RESPONSE_DESCRIPTION "TODO: create_response description"
#define CREATE_RESPONSE_ACTION M_OUTPUT
#define CREATE_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void create_response (aid_t);

typedef action_traits<input_action, auto_parameter> bind_request_traits;
#define BIND_REQUEST_TRAITS bind_request_traits
#define BIND_REQUEST_NAME "bind_request"
#define BIND_REQUEST_DESCRIPTION "TODO: bind_request description"
#define BIND_REQUEST_ACTION M_INPUT
#define BIND_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void bind_request (aid_t, void*, size_t, bid_t, size_t);

typedef action_traits<internal_action> bind_traits;
#define BIND_TRAITS bind_traits
#define BIND_NAME "bind"
#define BIND_DESCRIPTION ""
#define BIND_ACTION M_INTERNAL
#define BIND_PARAMETER M_NO_PARAMETER
extern "C" void bind (no_param_t);

typedef action_traits<output_action, auto_parameter> bind_response_traits;
#define BIND_RESPONSE_TRAITS bind_response_traits
#define BIND_RESPONSE_NAME "bind_response"
#define BIND_RESPONSE_DESCRIPTION "TODO: bind_response description"
#define BIND_RESPONSE_ACTION M_OUTPUT
#define BIND_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void bind_response (aid_t);

typedef action_traits<input_action, auto_parameter> loose_request_traits;
#define LOOSE_REQUEST_TRAITS loose_request_traits
#define LOOSE_REQUEST_NAME "loose_request"
#define LOOSE_REQUEST_DESCRIPTION "TODO: loose_request description"
#define LOOSE_REQUEST_ACTION M_INPUT
#define LOOSE_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void loose_request (aid_t, void*, size_t, bid_t, size_t);

typedef action_traits<internal_action> loose_traits;
#define LOOSE_TRAITS loose_traits
#define LOOSE_NAME "loose"
#define LOOSE_DESCRIPTION ""
#define LOOSE_ACTION M_INTERNAL
#define LOOSE_PARAMETER M_NO_PARAMETER
extern "C" void loose (no_param_t);

typedef action_traits<output_action, auto_parameter> loose_response_traits;
#define LOOSE_RESPONSE_TRAITS loose_response_traits
#define LOOSE_RESPONSE_NAME "loose_response"
#define LOOSE_RESPONSE_DESCRIPTION "TODO: loose_response description"
#define LOOSE_RESPONSE_ACTION M_OUTPUT
#define LOOSE_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void loose_response (aid_t);

typedef action_traits<input_action, auto_parameter> destroy_request_traits;
#define DESTROY_REQUEST_TRAITS destroy_request_traits
#define DESTROY_REQUEST_NAME "destroy_request"
#define DESTROY_REQUEST_DESCRIPTION "TODO: destroy_request description"
#define DESTROY_REQUEST_ACTION M_INPUT
#define DESTROY_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void destroy_request (aid_t, void*, size_t, bid_t, size_t);

typedef action_traits<internal_action> destroy_traits;
#define DESTROY_TRAITS destroy_traits
#define DESTROY_NAME "destroy"
#define DESTROY_DESCRIPTION ""
#define DESTROY_ACTION M_INTERNAL
#define DESTROY_PARAMETER M_NO_PARAMETER
extern "C" void destroy (no_param_t);

typedef action_traits<output_action, auto_parameter> destroy_response_traits;
#define DESTROY_RESPONSE_TRAITS destroy_response_traits
#define DESTROY_RESPONSE_NAME "destroy_response"
#define DESTROY_RESPONSE_DESCRIPTION "TODO: destroy_response description"
#define DESTROY_RESPONSE_ACTION M_OUTPUT
#define DESTROY_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void destroy_response (aid_t);

#endif /* __system_automaton_hpp__ */
