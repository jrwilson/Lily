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

namespace system_automaton {

  typedef action_traits<input_action<equal>, auto_parameter> create_request_traits;
#define SA_CREATE_REQUEST_TRAITS system_automaton::create_request_traits
#define SA_CREATE_REQUEST_NAME "create_request"
#define SA_CREATE_REQUEST_DESCRIPTION "TODO: create_request description"
#define SA_CREATE_REQUEST_COMPARE M_EQUAL
#define SA_CREATE_REQUEST_ACTION M_INPUT
#define SA_CREATE_REQUEST_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<output_action<equal>, auto_parameter> init_traits;
#define SA_INIT_TRAITS system_automaton::init_traits
#define SA_INIT_NAME "init"
#define SA_INIT_DESCRIPTION "TODO: init description"
#define SA_INIT_COMPARE M_EQUAL
#define SA_INIT_ACTION M_OUTPUT
#define SA_INIT_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<output_action<equal>, auto_parameter> create_response_traits;
#define SA_CREATE_RESPONSE_TRAITS system_automaton::create_response_traits
#define SA_CREATE_RESPONSE_NAME "create_response"
#define SA_CREATE_RESPONSE_DESCRIPTION "TODO: create_response description"
#define SA_CREATE_RESPONSE_COMPARE M_EQUAL
#define SA_CREATE_RESPONSE_ACTION M_OUTPUT
#define SA_CREATE_RESPONSE_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<input_action<equal>, auto_parameter> bind_request_traits;
#define SA_BIND_REQUEST_TRAITS system_automaton::bind_request_traits
#define SA_BIND_REQUEST_NAME "bind_request"
#define SA_BIND_REQUEST_DESCRIPTION "TODO: bind_request description"
#define SA_BIND_REQUEST_COMPARE M_EQUAL
#define SA_BIND_REQUEST_ACTION M_INPUT
#define SA_BIND_REQUEST_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<output_action<equal>, auto_parameter> bind_response_traits;
#define SA_BIND_RESPONSE_TRAITS system_automaton::bind_response_traits
#define SA_BIND_RESPONSE_NAME "bind_response"
#define SA_BIND_RESPONSE_DESCRIPTION "TODO: bind_response description"
#define SA_BIND_RESPONSE_COMPARE M_EQUAL
#define SA_BIND_RESPONSE_ACTION M_OUTPUT
#define SA_BIND_RESPONSE_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<input_action<equal>, auto_parameter> loose_request_traits;
#define SA_LOOSE_REQUEST_TRAITS system_automaton::loose_request_traits
#define SA_LOOSE_REQUEST_NAME "loose_request"
#define SA_LOOSE_REQUEST_DESCRIPTION "TODO: loose_request description"
#define SA_LOOSE_REQUEST_COMPARE M_EQUAL
#define SA_LOOSE_REQUEST_ACTION M_INPUT
#define SA_LOOSE_REQUEST_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<output_action<equal>, auto_parameter> loose_response_traits;
#define SA_LOOSE_RESPONSE_TRAITS system_automaton::loose_response_traits
#define SA_LOOSE_RESPONSE_NAME "loose_response"
#define SA_LOOSE_RESPONSE_DESCRIPTION "TODO: loose_response description"
#define SA_LOOSE_RESPONSE_COMPARE M_EQUAL
#define SA_LOOSE_RESPONSE_ACTION M_OUTPUT
#define SA_LOOSE_RESPONSE_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<input_action<equal>, auto_parameter> destroy_request_traits;
#define SA_DESTROY_REQUEST_TRAITS system_automaton::destroy_request_traits
#define SA_DESTROY_REQUEST_NAME "destroy_request"
#define SA_DESTROY_REQUEST_DESCRIPTION "TODO: destroy_request description"
#define SA_DESTROY_REQUEST_COMPARE M_EQUAL
#define SA_DESTROY_REQUEST_ACTION M_INPUT
#define SA_DESTROY_REQUEST_PARAMETER M_AUTO_PARAMETER

  typedef action_traits<output_action<equal>, auto_parameter> destroy_response_traits;
#define SA_DESTROY_RESPONSE_TRAITS system_automaton::destroy_response_traits
#define SA_DESTROY_RESPONSE_NAME "destroy_response"
#define SA_DESTROY_RESPONSE_DESCRIPTION "TODO: destroy_response description"
#define SA_DESTROY_RESPONSE_COMPARE M_EQUAL
#define SA_DESTROY_RESPONSE_ACTION M_OUTPUT
#define SA_DESTROY_RESPONSE_PARAMETER M_AUTO_PARAMETER

}

#endif /* __system_automaton_hpp__ */
