#include "action_test_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kassert.hpp"
#include <string.h>

struct action_test_automaton_allocator_tag { };
typedef list_alloc<action_test_automaton_allocator_tag> alloc_type;

template <typename T>
struct allocator_type : public list_allocator<T, action_test_automaton_allocator_tag> { };

namespace action_test {

  aid_t ap_nb_nc_input1_parameter;
  aid_t ap_nb_nc_input2_parameter;
  aid_t ap_nb_nc_input3_parameter;

  aid_t ap_nb_c_input1_parameter;
  aid_t ap_nb_c_input2_parameter;
  aid_t ap_nb_c_input3_parameter;

  aid_t ap_b_nc_input1_parameter;
  aid_t ap_b_nc_input2_parameter;
  aid_t ap_b_nc_input3_parameter;

  aid_t ap_b_c_input1_parameter;
  aid_t ap_b_c_input2_parameter;
  aid_t ap_b_c_input3_parameter;

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static bool np_nb_nc_output_flag = true;
  static bool np_nb_c_output_flag = true;
  static const aid_t np_nb_c_output_value = 101;
  static bool np_b_nc_output_flag = true;
  static const aid_t np_b_nc_output_value = 101;
  static bool np_b_c_output_flag = true;
  static const aid_t np_b_c_output_value = 101;
  static bool p_nb_nc_output_flag = true;
  static bool p_nb_c_output_flag = true;
  static const aid_t p_nb_c_output_value = 104;
  static bool p_b_nc_output_flag = true;
  static const aid_t p_b_nc_output_value = 104;
  static bool p_b_c_output_flag = true;
  static const aid_t p_b_c_output_value = 104;
  static bool ap_nb_nc_output_flag = true;
  aid_t ap_nb_nc_output_parameter;
  static bool ap_nb_c_output_flag = true;
  aid_t ap_nb_c_output_parameter;
  static const aid_t ap_nb_c_output_value = 107;
  static bool ap_b_nc_output_flag = true;
  aid_t ap_b_nc_output_parameter;
  static const aid_t ap_b_nc_output_value = 107;
  static bool ap_b_c_output_flag = true;
  aid_t ap_b_c_output_parameter;
  static const aid_t ap_b_c_output_value = 107;

  static bool np_internal_flag = true;
  static const aid_t p_internal_parameter = 108;
  static bool p_internal_flag = true;
    
  static void
  schedule ();
  
  void
  init ()
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();
    schedule ();
    scheduler_->finish<init_traits> ();
  }

  static void
  no_schedule () { }

  static void
  no_finish () {
    syscall::finish (reinterpret_cast<const void*> (-1), -1, false, -1, 0);
  }

  void
  np_nb_nc_input1 ()
  {
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_nb_nc_input2 ()
  {
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_nb_nc_input3 ()
  {
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_nb_c_input1 (aid_t v)
  {
    kassert (v == np_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_nb_c_input2 (aid_t v)
  {
    kassert (v == p_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_nb_c_input3 (aid_t v)
  {
    kassert (v == ap_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_b_nc_input1 (bid_t b)
  {
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "np_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_b_nc_input2 (bid_t b)
  {
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "p_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_b_nc_input3 (bid_t b)
  {
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "ap_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_b_c_input1 (bid_t b,
		 aid_t v)
  {
    kassert (v == np_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "np_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_b_c_input2 (bid_t b,
		 aid_t v)
  {
    kassert (v == p_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "p_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  np_b_c_input3 (bid_t b,
		 aid_t v)
  {
    kassert (v == ap_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "ap_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }
  
  void
  p_nb_nc_input1 (aid_t p)
  {
    kassert (p == p_nb_nc_input1_parameter);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_nb_nc_input2 (aid_t p)
  {
    kassert (p == p_nb_nc_input2_parameter);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_nb_nc_input3 (aid_t p)
  {
    kassert (p == p_nb_nc_input3_parameter);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_nb_c_input1 (aid_t p,
		 aid_t v)
  {
    kassert (p == p_nb_c_input1_parameter);
    kassert (v == np_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_nb_c_input2 (aid_t p,
		 aid_t v)
  {
    kassert (p == p_nb_c_input2_parameter);
    kassert (v == p_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_nb_c_input3 (aid_t p,
		 aid_t v)
  {
    kassert (p == p_nb_c_input3_parameter);
    kassert (v == ap_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_b_nc_input1 (aid_t p,
		 bid_t b)
  {
    kassert (p == p_b_nc_input1_parameter);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "np_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_b_nc_input2 (aid_t p,
		 bid_t b)
  {
    kassert (p == p_b_nc_input2_parameter);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "p_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_b_nc_input3 (aid_t p,
		 bid_t b)
  {
    kassert (p == p_b_nc_input3_parameter);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "ap_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_b_c_input1 (aid_t p,
		bid_t b,
		aid_t v)
  {
    kassert (p == p_b_c_input1_parameter);
    kassert (v == np_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "np_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_b_c_input2 (aid_t p,
		bid_t b,
		aid_t v)
  {
    kassert (p == p_b_c_input2_parameter);
    kassert (v == p_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "p_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  p_b_c_input3 (aid_t p,
		bid_t b,
		aid_t v)
  {
    kassert (p == p_b_c_input3_parameter);
    kassert (v == ap_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "ap_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_nb_nc_input1 (aid_t p)
  {
    kassert (p == ap_nb_nc_input1_parameter);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_nb_nc_input2 (aid_t p)
  {
    kassert (p == ap_nb_nc_input2_parameter);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_nb_nc_input3 (aid_t p)
  {
    kassert (p == ap_nb_nc_input3_parameter);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_nb_c_input1 (aid_t p,
		  aid_t v)
  {
    kassert (p == ap_nb_c_input1_parameter);
    kassert (v == np_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_nb_c_input2 (aid_t p,
		  aid_t v)
  {
    kassert (p == ap_nb_c_input2_parameter);
    kassert (v == p_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_nb_c_input3 (aid_t p,
		  aid_t v)
  {
    kassert (p == ap_nb_c_input3_parameter);
    kassert (v == ap_nb_c_output_value);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_b_nc_input1 (aid_t p,
		  bid_t b)
  {
    kassert (p == ap_b_nc_input1_parameter);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "np_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_b_nc_input2 (aid_t p,
		  bid_t b)
  {
    kassert (p == ap_b_nc_input2_parameter);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "p_b_nc_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_b_nc_input3 (aid_t p,
		  bid_t b)
  {
    kassert (p == ap_b_nc_input3_parameter);
    kout << "\t" << __func__;
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "ap_b_nc_output") == 0);
    no_schedule ();
    no_finish ();
  }

  void
  ap_b_c_input1 (aid_t p,
		 bid_t b,
		 aid_t v)
  {
    kassert (p == ap_b_c_input1_parameter);
    kassert (v == np_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "np_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_b_c_input2 (aid_t p,
		 bid_t b,
		 aid_t v)
  {
    kassert (p == ap_b_c_input2_parameter);
    kassert (v == p_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "p_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  void
  ap_b_c_input3 (aid_t p,
		 bid_t b,
		 aid_t v)
  {
    kassert (p == ap_b_c_input3_parameter);
    kassert (v == ap_b_c_output_value);
    char* c = static_cast<char*> (syscall::buffer_map (b));
    kassert (c != reinterpret_cast<const void*> (-1));
    kassert (strcmp (c, "ap_b_c_output") == 0);
    kout << "\t" << __func__;
    no_schedule ();
    no_finish ();
  }

  static bool
  np_nb_nc_output_precondition ()
  {
    return np_nb_nc_output_flag;
  }

  void
  np_nb_nc_output ()
  {
    scheduler_->remove<np_nb_nc_output_traits> (&np_nb_nc_output);
    if (np_nb_nc_output_precondition ()) {
      np_nb_nc_output_flag = false;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<np_nb_nc_output_traits> (true);
    }
    else {
      schedule ();
      scheduler_->finish<np_nb_nc_output_traits> (false);
    }
  }

  static bool
  np_nb_c_output_precondition ()
  {
    return np_nb_c_output_flag;
  }

  void
  np_nb_c_output ()
  {
    scheduler_->remove<np_nb_c_output_traits> (&np_nb_c_output);
    if (np_nb_c_output_precondition ()) {
      np_nb_c_output_flag = false;
      static aid_t value = np_nb_c_output_value;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<np_nb_c_output_traits> (&value);
    }
    else {
      schedule ();
      scheduler_->finish<np_nb_c_output_traits> ();
    }
  }

  static bool
  np_b_nc_output_precondition ()
  {
    return np_b_nc_output_flag;
  }

  bid_t
  string_to_buffer (const char* s)
  {
    size_t len = strlen (s);
    bid_t b = syscall::buffer_create (strlen (s));
    char* c = static_cast<char*> (syscall::buffer_map (b));
    memcpy (c, s, len);
    return b;
  }

  void
  np_b_nc_output ()
  {
    scheduler_->remove<np_b_nc_output_traits> (&np_b_nc_output);
    if (np_b_nc_output_precondition ()) {
      np_b_nc_output_flag = false;
      bid_t b = string_to_buffer (__func__);
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<np_b_nc_output_traits> (b);
    }
    else {
      schedule ();
      scheduler_->finish<np_b_nc_output_traits> ();
    }
  }

  static bool
  np_b_c_output_precondition ()
  {
    return np_b_c_output_flag;
  }

  void
  np_b_c_output ()
  {
    scheduler_->remove<np_b_c_output_traits> (&np_b_c_output);
    if (np_b_c_output_precondition ()) {
      np_b_c_output_flag = false;
      bid_t b = string_to_buffer (__func__);
      static aid_t value = np_b_c_output_value;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<np_b_c_output_traits> (b, &value);
    }
    else {
      schedule ();
      scheduler_->finish<np_b_c_output_traits> ();
    }
  }

  static bool
  p_nb_nc_output_precondition (aid_t p)
  {
    kassert (p == p_nb_nc_output_parameter);
    return p_nb_nc_output_flag;
  }

  void
  p_nb_nc_output (aid_t p)
  {
    scheduler_->remove<p_nb_nc_output_traits> (&p_nb_nc_output, p);
    if (p_nb_nc_output_precondition (p)) {
      kassert (p == p_nb_nc_output_parameter);
      p_nb_nc_output_flag = false;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<p_nb_nc_output_traits> (true);
    }
    else {
      schedule ();
      scheduler_->finish<p_nb_nc_output_traits> (false);
    }
  }

  static bool
  p_nb_c_output_precondition (aid_t p)
  {
    kassert (p == p_nb_c_output_parameter);
    return p_nb_c_output_flag;
  }

  void
  p_nb_c_output (aid_t p)
  {
    scheduler_->remove<p_nb_c_output_traits> (&p_nb_c_output, p);
    if (p_nb_c_output_precondition (p)) {
      kassert (p == p_nb_c_output_parameter);
      p_nb_c_output_flag = false;
      static aid_t value = p_nb_c_output_value;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<p_nb_c_output_traits> (&value);
    }
    else {
      schedule ();
      scheduler_->finish<p_nb_c_output_traits> ();
    }
  }

  static bool
  p_b_nc_output_precondition (aid_t p)
  {
    kassert (p == p_b_nc_output_parameter);
    return p_b_nc_output_flag;
  }

  void
  p_b_nc_output (aid_t p)
  {
    scheduler_->remove<p_b_nc_output_traits> (&p_b_nc_output, p);
    if (p_b_nc_output_precondition (p)) {
      kassert (p == p_b_nc_output_parameter);
      p_b_nc_output_flag = false;
      bid_t b = string_to_buffer (__func__);
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<p_b_nc_output_traits> (b);
    }
    else {
      schedule ();
      scheduler_->finish<p_b_nc_output_traits> ();
    }
  }

  static bool
  p_b_c_output_precondition (aid_t p)
  {
    kassert (p == p_b_c_output_parameter);
    return p_b_c_output_flag;
  }

  void
  p_b_c_output (aid_t p)
  {
    scheduler_->remove<p_b_c_output_traits> (&p_b_c_output, p);
    if (p_b_c_output_precondition (p)) {
      kassert (p == p_b_c_output_parameter);
      p_b_c_output_flag = false;
      bid_t b = string_to_buffer (__func__);
      static aid_t value = p_b_c_output_value;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<p_b_c_output_traits> (b, &value);
    }
    else {
      schedule ();
      scheduler_->finish<p_b_c_output_traits> ();
    }
  }

  static bool
  ap_nb_nc_output_precondition (aid_t p)
  {
    kassert (p == ap_nb_nc_output_parameter);
    return ap_nb_nc_output_flag;
  }

  void
  ap_nb_nc_output (aid_t p)
  {
    scheduler_->remove<ap_nb_nc_output_traits> (&ap_nb_nc_output, p);
    if (ap_nb_nc_output_precondition (p)) {
      kassert (p == ap_nb_nc_output_parameter);
      ap_nb_nc_output_flag = false;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<ap_nb_nc_output_traits> (true);
    }
    else {
      schedule ();
      scheduler_->finish<ap_nb_nc_output_traits> (false);
    }
  }

  static bool
  ap_nb_c_output_precondition (aid_t p)
  {
    kassert (p == ap_nb_c_output_parameter);
    return ap_nb_c_output_flag;
  }

  void
  ap_nb_c_output (aid_t p)
  {
    scheduler_->remove<ap_nb_c_output_traits> (&ap_nb_c_output, p);
    if (ap_nb_c_output_precondition (p)) {
      kassert (p == ap_nb_c_output_parameter);
      ap_nb_c_output_flag = false;
      static aid_t value = ap_nb_c_output_value;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<ap_nb_c_output_traits> (&value);
    }
    else {
      schedule ();
      scheduler_->finish<ap_nb_c_output_traits> ();
    }
  }

  static bool
  ap_b_nc_output_precondition (aid_t p)
  {
    kassert (p == ap_b_nc_output_parameter);
    return ap_b_nc_output_flag;
  }

  void
  ap_b_nc_output (aid_t p)
  {
    scheduler_->remove<ap_b_nc_output_traits> (&ap_b_nc_output, p);
    if (ap_b_nc_output_precondition (p)) {
      kassert (p == ap_b_nc_output_parameter);
      ap_b_nc_output_flag = false;
      bid_t b = string_to_buffer (__func__);
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<ap_b_nc_output_traits> (b);
    }
    else {
      schedule ();
      scheduler_->finish<ap_b_nc_output_traits> ();
    }
  }

  static bool
  ap_b_c_output_precondition (aid_t p)
  {
    kassert (p == ap_b_c_output_parameter);
    return ap_b_c_output_flag;
  }

  void
  ap_b_c_output (aid_t p)
  {
    scheduler_->remove<ap_b_c_output_traits> (&ap_b_c_output, p);
    if (ap_b_c_output_precondition (p)) {
      kassert (p == ap_b_c_output_parameter);
      ap_b_c_output_flag = false;
      bid_t b = string_to_buffer (__func__);
      static aid_t value = ap_b_c_output_value;
      kout << endl << __func__;
      schedule ();
      scheduler_->finish<ap_b_c_output_traits> (b, &value);
    }
    else {
      schedule ();
      scheduler_->finish<ap_b_c_output_traits> ();
    }
  }

  static bool
  np_internal_precondition ()
  {
    return np_internal_flag;
  }

  void
  np_internal ()
  {
    scheduler_->remove<np_internal_traits> (&np_internal);
    if (np_internal_precondition ()) {
      np_internal_flag = false;
      kout << endl << __func__;
    }
    schedule ();
    scheduler_->finish<np_internal_traits> ();
  }

  static bool
  p_internal_precondition (aid_t p)
  {
    kassert (p == p_internal_parameter);
    return p_internal_flag;
  }

  void
  p_internal (int p)
  {
    scheduler_->remove<p_internal_traits> (&p_internal, p);
    if (p_internal_precondition (p)) {
      kassert (p == p_internal_parameter);
      p_internal_flag = false;
      kout << endl << __func__;
    }
    schedule ();
    scheduler_->finish<p_internal_traits> ();
  }

  static void
  schedule ()
  {
    if (np_nb_nc_output_precondition ()) {
      scheduler_->add<np_nb_nc_output_traits> (&np_nb_nc_output);
    }
    if (np_nb_c_output_precondition ()) {
      scheduler_->add<np_nb_c_output_traits> (&np_nb_c_output);
    }
    if (np_b_nc_output_precondition ()) {
      scheduler_->add<np_b_nc_output_traits> (&np_b_nc_output);
    }
    if (np_b_c_output_precondition ()) {
      scheduler_->add<np_b_c_output_traits> (&np_b_c_output);
    }
    if (p_nb_nc_output_precondition (p_nb_nc_output_parameter)) {
      scheduler_->add<p_nb_nc_output_traits> (&p_nb_nc_output, p_nb_nc_output_parameter);
    }
    if (p_nb_c_output_precondition (p_nb_c_output_parameter)) {
      scheduler_->add<p_nb_c_output_traits> (&p_nb_c_output, p_nb_c_output_parameter);
    }
    if (p_b_nc_output_precondition (p_b_nc_output_parameter)) {
      scheduler_->add<p_b_nc_output_traits> (&p_b_nc_output, p_b_nc_output_parameter);
    }
    if (p_b_c_output_precondition (p_b_c_output_parameter)) {
      scheduler_->add<p_b_c_output_traits> (&p_b_c_output, p_b_c_output_parameter);
    }
    if (ap_nb_nc_output_precondition (ap_nb_nc_output_parameter)) {
      scheduler_->add<ap_nb_nc_output_traits> (&ap_nb_nc_output, ap_nb_nc_output_parameter);
    }
    if (ap_nb_c_output_precondition (ap_nb_c_output_parameter)) {
      scheduler_->add<ap_nb_c_output_traits> (&ap_nb_c_output, ap_nb_c_output_parameter);
    }
    if (ap_b_nc_output_precondition (ap_b_nc_output_parameter)) {
      scheduler_->add<ap_b_nc_output_traits> (&ap_b_nc_output, ap_b_nc_output_parameter);
    }
    if (ap_b_c_output_precondition (ap_b_c_output_parameter)) {
      scheduler_->add<ap_b_c_output_traits> (&ap_b_c_output, ap_b_c_output_parameter);
    }
    if (np_internal_precondition ()) {
      scheduler_->add<np_internal_traits> (&np_internal);
    }
    if (p_internal_precondition (p_internal_parameter)) {
      scheduler_->add<p_internal_traits> (&p_internal, p_internal_parameter);
    }
  }

}
