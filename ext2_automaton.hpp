#ifndef __ext2_automaton_hpp__
#define __ext2_automaton_hpp__

/*
  File
  ----
  ext2_automaton.hpp
  
  Description
  -----------
  Declarations for an ext2 filesystem.

  Authors:
  Justin R. Wilson
*/

#include "action_traits.hpp"
#include "ramdisk_automaton.hpp"

namespace ext2 {

  // Init.
  typedef np_nb_nc_input_action_traits init_traits;  
  void
  init (void);

  // Info request.
  typedef np_nb_nc_output_action_traits info_request_traits;
  void
  info_request ();

  typedef np_nb_c_input_action_traits<ramdisk::info_response_t> info_response_traits;
  void
  info_response (ramdisk::info_response_t);

  typedef np_nb_c_output_action_traits<ramdisk::read_request_t> read_request_traits;
  void
  read_request ();

  typedef np_b_c_input_action_traits<void, ramdisk::read_response_t> read_response_traits;
  void
  read_response (bid_t,
		 ramdisk::read_response_t);

  typedef np_b_c_output_action_traits<void, ramdisk::write_request_t> write_request_traits;
  void
  write_request ();
  
  typedef np_nb_c_input_action_traits<ramdisk::write_response_t> write_response_traits;
  void
  write_response (ramdisk::write_response_t);


  typedef np_internal_action_traits generate_read_request_traits;
  void
  generate_read_request ();
}

#endif /* __ext2_automaton_hpp__ */

