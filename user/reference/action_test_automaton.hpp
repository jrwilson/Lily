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

/*
  This table shows the 36 possible binding combinations.
  
            np_nb_nc np_nb_c np_b_nc np_b_c p_nb_nc p_nb_c p_b_nc p_b_c ap_nb_nc ap_nb_c ap_b_nc ap_b_c
  np_nb_nc      X                              X                            X
  np_nb_c                X                             X                             X
  np_b_nc                       X                            X                              X
  np_b_c                                X                           X                               X
  p_nb_nc       X                              X                            X
  p_nb_c                 X                             X                             X
  p_b_nc                        X                            X                              X
  p_b_c                                 X                           X                               X
  ap_nb_nc      X                              X                            X
  ap_nb_c                X                             X                             X
  ap_b_nc                       X                            X                              X
  ap_b_c                                X                           X                               X


 */

namespace action_test {
  
  static const aid_t p_nb_nc_input1_parameter = 500;
  static const aid_t p_nb_nc_input2_parameter = 501;
  static const aid_t p_nb_nc_input3_parameter = 502;

  static const aid_t p_nb_c_input1_parameter = 503;
  static const aid_t p_nb_c_input2_parameter = 504;
  static const aid_t p_nb_c_input3_parameter = 505;

  static const aid_t p_b_nc_input1_parameter = 506;
  static const aid_t p_b_nc_input2_parameter = 507;
  static const aid_t p_b_nc_input3_parameter = 508;

  static const aid_t p_b_c_input1_parameter = 509;
  static const aid_t p_b_c_input2_parameter = 510;
  static const aid_t p_b_c_input3_parameter = 511;

  extern aid_t ap_nb_nc_input1_parameter;
  extern aid_t ap_nb_nc_input2_parameter;
  extern aid_t ap_nb_nc_input3_parameter;

  extern aid_t ap_nb_c_input1_parameter;
  extern aid_t ap_nb_c_input2_parameter;
  extern aid_t ap_nb_c_input3_parameter;

  extern aid_t ap_b_nc_input1_parameter;
  extern aid_t ap_b_nc_input2_parameter;
  extern aid_t ap_b_nc_input3_parameter;

  extern aid_t ap_b_c_input1_parameter;
  extern aid_t ap_b_c_input2_parameter;
  extern aid_t ap_b_c_input3_parameter;

  static const aid_t p_nb_nc_output_parameter = 512;
  extern aid_t ap_nb_nc_output_parameter;
  static const aid_t p_nb_c_output_parameter = 513;
  extern aid_t ap_nb_c_output_parameter;
  static const aid_t p_b_nc_output_parameter = 514;
  extern aid_t ap_b_nc_output_parameter;
  static const aid_t p_b_c_output_parameter = 515;
  extern aid_t ap_b_c_output_parameter;

  typedef np_nb_nc_input_action_traits init_traits;
  void
  init ();

  typedef np_nb_nc_input_action_traits np_nb_nc_input1_traits;
  void
  np_nb_nc_input1 ();

  typedef np_nb_nc_input_action_traits np_nb_nc_input2_traits;
  void
  np_nb_nc_input2 ();

  typedef np_nb_nc_input_action_traits np_nb_nc_input3_traits;
  void
  np_nb_nc_input3 ();

  typedef np_nb_c_input_action_traits<aid_t> np_nb_c_input1_traits;
  void
  np_nb_c_input1 (aid_t);

  typedef np_nb_c_input_action_traits<aid_t> np_nb_c_input2_traits;
  void
  np_nb_c_input2 (aid_t);

  typedef np_nb_c_input_action_traits<aid_t> np_nb_c_input3_traits;
  void
  np_nb_c_input3 (aid_t);

  typedef np_b_nc_input_action_traits<char*> np_b_nc_input1_traits;
  void
  np_b_nc_input1 (bid_t);

  typedef np_b_nc_input_action_traits<char*> np_b_nc_input2_traits;
  void
  np_b_nc_input2 (bid_t);

  typedef np_b_nc_input_action_traits<char*> np_b_nc_input3_traits;
  void
  np_b_nc_input3 (bid_t);

  typedef np_b_c_input_action_traits<char*, aid_t> np_b_c_input1_traits;
  void
  np_b_c_input1 (bid_t, aid_t);

  typedef np_b_c_input_action_traits<char*, aid_t> np_b_c_input2_traits;
  void
  np_b_c_input2 (bid_t, aid_t);

  typedef np_b_c_input_action_traits<char*, aid_t> np_b_c_input3_traits;
  void
  np_b_c_input3 (bid_t, aid_t);

  typedef p_nb_nc_input_action_traits<aid_t> p_nb_nc_input1_traits;
  void
  p_nb_nc_input1 (aid_t);

  typedef p_nb_nc_input_action_traits<aid_t> p_nb_nc_input2_traits;
  void
  p_nb_nc_input2 (aid_t);

  typedef p_nb_nc_input_action_traits<aid_t> p_nb_nc_input3_traits;
  void
  p_nb_nc_input3 (aid_t);

  typedef p_nb_c_input_action_traits<aid_t, aid_t> p_nb_c_input1_traits;
  void
  p_nb_c_input1 (aid_t, aid_t);

  typedef p_nb_c_input_action_traits<aid_t, aid_t> p_nb_c_input2_traits;
  void
  p_nb_c_input2 (aid_t, aid_t);

  typedef p_nb_c_input_action_traits<aid_t, aid_t> p_nb_c_input3_traits;
  void
  p_nb_c_input3 (aid_t, aid_t);

  typedef p_b_nc_input_action_traits<aid_t, char*> p_b_nc_input1_traits;
  void
  p_b_nc_input1 (aid_t, bid_t);

  typedef p_b_nc_input_action_traits<aid_t, char*> p_b_nc_input2_traits;
  void
  p_b_nc_input2 (aid_t, bid_t);

  typedef p_b_nc_input_action_traits<aid_t, char*> p_b_nc_input3_traits;
  void
  p_b_nc_input3 (aid_t, bid_t);

  typedef p_b_c_input_action_traits<aid_t, char*, aid_t> p_b_c_input1_traits;
  void
  p_b_c_input1 (aid_t, bid_t, aid_t);

  typedef p_b_c_input_action_traits<aid_t, char*, aid_t> p_b_c_input2_traits;
  void
  p_b_c_input2 (aid_t, bid_t, aid_t);

  typedef p_b_c_input_action_traits<aid_t, char*, aid_t> p_b_c_input3_traits;
  void
  p_b_c_input3 (aid_t, bid_t, aid_t);

  typedef ap_nb_nc_input_action_traits ap_nb_nc_input1_traits;
  void
  ap_nb_nc_input1 (aid_t);

  typedef ap_nb_nc_input_action_traits ap_nb_nc_input2_traits;
  void
  ap_nb_nc_input2 (aid_t);

  typedef ap_nb_nc_input_action_traits ap_nb_nc_input3_traits;
  void
  ap_nb_nc_input3 (aid_t);

  typedef ap_nb_c_input_action_traits<aid_t> ap_nb_c_input1_traits;
  void
  ap_nb_c_input1 (aid_t, aid_t);

  typedef ap_nb_c_input_action_traits<aid_t> ap_nb_c_input2_traits;
  void
  ap_nb_c_input2 (aid_t, aid_t);

  typedef ap_nb_c_input_action_traits<aid_t> ap_nb_c_input3_traits;
  void
  ap_nb_c_input3 (aid_t, aid_t);

  typedef ap_b_nc_input_action_traits<char*> ap_b_nc_input1_traits;
  void
  ap_b_nc_input1 (aid_t, bid_t);

  typedef ap_b_nc_input_action_traits<char*> ap_b_nc_input2_traits;
  void
  ap_b_nc_input2 (aid_t, bid_t);

  typedef ap_b_nc_input_action_traits<char*> ap_b_nc_input3_traits;
  void
  ap_b_nc_input3 (aid_t, bid_t);

  typedef ap_b_c_input_action_traits<char*, aid_t> ap_b_c_input1_traits;
  void
  ap_b_c_input1 (aid_t, bid_t, aid_t);

  typedef ap_b_c_input_action_traits<char*, aid_t> ap_b_c_input2_traits;
  void
  ap_b_c_input2 (aid_t, bid_t, aid_t);

  typedef ap_b_c_input_action_traits<char*, aid_t> ap_b_c_input3_traits;
  void
  ap_b_c_input3 (aid_t, bid_t, aid_t);

  typedef np_nb_nc_output_action_traits np_nb_nc_output_traits;
  void
  np_nb_nc_output ();

  typedef np_nb_c_output_action_traits<aid_t> np_nb_c_output_traits;
  void
  np_nb_c_output ();

  typedef np_b_nc_output_action_traits<char*> np_b_nc_output_traits;
  void
  np_b_nc_output ();

  typedef np_b_c_output_action_traits<char*, aid_t> np_b_c_output_traits;
  void
  np_b_c_output ();

  typedef p_nb_nc_output_action_traits<aid_t> p_nb_nc_output_traits;
  void
  p_nb_nc_output (aid_t);

  typedef p_nb_c_output_action_traits<aid_t, aid_t> p_nb_c_output_traits;
  void
  p_nb_c_output (aid_t);

  typedef p_b_nc_output_action_traits<aid_t, char*> p_b_nc_output_traits;
  void
  p_b_nc_output (aid_t);

  typedef p_b_c_output_action_traits<aid_t, char*, aid_t> p_b_c_output_traits;
  void
  p_b_c_output (aid_t);

  typedef ap_nb_nc_output_action_traits ap_nb_nc_output_traits;
  void
  ap_nb_nc_output (aid_t);

  typedef ap_nb_c_output_action_traits<aid_t> ap_nb_c_output_traits;
  void
  ap_nb_c_output (aid_t);

  typedef ap_b_nc_output_action_traits<char*> ap_b_nc_output_traits;
  void
  ap_b_nc_output (aid_t);

  typedef ap_b_c_output_action_traits<char*, aid_t> ap_b_c_output_traits;
  void
  ap_b_c_output (aid_t);

  typedef np_internal_action_traits np_internal_traits;
  void
  np_internal ();

  typedef p_internal_action_traits<aid_t> p_internal_traits;
  void
  p_internal (aid_t);

}

#endif /* __action_test_automaton_hpp__ */

