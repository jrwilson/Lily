#ifndef __ramdisk_automaton_hpp__
#define __ramdisk_automaton_hpp__

/*
  File
  ----
  ramdisk_automaton.hpp
  
  Description
  -----------
  Declarations for a ramdisk.

  Authors:
  Justin R. Wilson
*/

#include "action_traits.hpp"

namespace ramdisk {

  typedef uint64_t block_num_t;

  enum error_t {
    SUCCESS,	// Request was successful.
    RANGE,	// Block was out of range.
  };

  extern bid_t bid;

  // Init.
  typedef np_nb_nc_input_action_traits init_traits;  
  void
  init (void);

  // Info request.
  typedef ap_nb_nc_input_action_traits info_request_traits;
  void
  info_request (aid_t);

  // Info response.
  struct info_response_t {
    block_num_t block_count;
  };

  typedef ap_nb_c_output_action_traits<info_response_t> info_response_traits;
  void
  info_response (aid_t);

  // Read request.
  struct read_request_t {
    block_num_t block_num;
  };

  typedef ap_nb_c_input_action_traits<read_request_t> read_request_traits;
  void
  read_request (aid_t,
		read_request_t);

  // Read response.
  struct read_response_t {
    block_num_t block_num;
    error_t error;
  };

  typedef ap_b_c_output_action_traits<void, read_response_t> read_response_traits;
  void
  read_response (aid_t);

  // Write request.
  struct write_request_t {
    block_num_t block_num;
  };
  
  typedef ap_b_c_input_action_traits<void, write_request_t> write_request_traits;
  void
  write_request (aid_t,
		 bid_t,
		 write_request_t);
  
  // Write response.
  struct write_response_t {
    block_num_t block_num;
    error_t error;
  };
  
  typedef ap_b_c_output_action_traits<void, write_response_t> write_response_traits;
  void
  write_response (aid_t);

}

#endif /* __ramdisk_automaton_hpp__ */

