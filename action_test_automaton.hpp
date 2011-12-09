#ifndef __action_test_automaton_hpp__
#define __action_test_automaton_hpp__

/*
  File
  ----
  action_test_automaton.hpp
  
  Description
  -----------
  Declarations for a action_test.

  Authors:
  Justin R. Wilson
*/

#include "action_traits.hpp"

namespace action_test {
  
  static const aid_t p_uv_input1_parameter = 500;
  static const aid_t p_uv_input2_parameter = 501;
  static const aid_t p_uv_input3_parameter = 502;

  static const aid_t p_v_input1_parameter = 503;
  static const aid_t p_v_input2_parameter = 504;
  static const aid_t p_v_input3_parameter = 505;

  extern aid_t ap_uv_input1_parameter;
  extern aid_t ap_uv_input2_parameter;
  extern aid_t ap_uv_input3_parameter;

  extern aid_t ap_v_input1_parameter;
  extern aid_t ap_v_input2_parameter;
  extern aid_t ap_v_input3_parameter;

  static const aid_t p_uv_output_parameter = 102;
  extern aid_t ap_uv_output_parameter;
  static const aid_t p_v_output_parameter = 103;
  extern aid_t ap_v_output_parameter;

  typedef up_uv_input_action_traits init_traits;
  void
  init ();

  typedef up_uv_input_action_traits up_uv_input1_traits;
  void
  up_uv_input1 ();

  typedef up_uv_input_action_traits up_uv_input2_traits;
  void
  up_uv_input2 ();

  typedef up_uv_input_action_traits up_uv_input3_traits;
  void
  up_uv_input3 ();

  typedef up_v_input_action_traits<aid_t> up_v_input1_traits;
  void
  up_v_input1 (aid_t);

  typedef up_v_input_action_traits<aid_t> up_v_input2_traits;
  void
  up_v_input2 (aid_t);

  typedef up_v_input_action_traits<aid_t> up_v_input3_traits;
  void
  up_v_input3 (aid_t);

  typedef p_uv_input_action_traits<aid_t> p_uv_input1_traits;
  void
  p_uv_input1 (aid_t);

  typedef p_uv_input_action_traits<aid_t> p_uv_input2_traits;
  void
  p_uv_input2 (aid_t);

  typedef p_uv_input_action_traits<aid_t> p_uv_input3_traits;
  void
  p_uv_input3 (aid_t);

  typedef p_v_input_action_traits<aid_t, aid_t> p_v_input1_traits;
  void
  p_v_input1 (aid_t, aid_t);

  typedef p_v_input_action_traits<aid_t, aid_t> p_v_input2_traits;
  void
  p_v_input2 (aid_t, aid_t);

  typedef p_v_input_action_traits<aid_t, aid_t> p_v_input3_traits;
  void
  p_v_input3 (aid_t, aid_t);

  typedef ap_uv_input_action_traits ap_uv_input1_traits;
  void
  ap_uv_input1 (aid_t);

  typedef ap_uv_input_action_traits ap_uv_input2_traits;
  void
  ap_uv_input2 (aid_t);

  typedef ap_uv_input_action_traits ap_uv_input3_traits;
  void
  ap_uv_input3 (aid_t);

  typedef ap_v_input_action_traits<aid_t> ap_v_input1_traits;
  void
  ap_v_input1 (aid_t, aid_t);

  typedef ap_v_input_action_traits<aid_t> ap_v_input2_traits;
  void
  ap_v_input2 (aid_t, aid_t);

  typedef ap_v_input_action_traits<aid_t> ap_v_input3_traits;
  void
  ap_v_input3 (aid_t, aid_t);

  typedef up_uv_output_action_traits up_uv_output_traits;
  void
  up_uv_output ();

  typedef up_v_output_action_traits<aid_t> up_v_output_traits;
  void
  up_v_output ();

  typedef p_uv_output_action_traits<aid_t> p_uv_output_traits;
  void
  p_uv_output (aid_t);

  typedef p_v_output_action_traits<aid_t, aid_t> p_v_output_traits;
  void
  p_v_output (aid_t);

  typedef ap_uv_output_action_traits ap_uv_output_traits;
  void
  ap_uv_output (aid_t);

  typedef ap_v_output_action_traits<aid_t> ap_v_output_traits;
  void
  ap_v_output (aid_t);

  typedef up_internal_action_traits up_internal_traits;
  void
  up_internal ();

  typedef p_internal_action_traits<aid_t> p_internal_traits;
  void
  p_internal (aid_t);

}

#endif /* __action_test_automaton_hpp__ */

