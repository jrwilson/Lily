#include "action_test_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kassert.hpp"

struct allocator_tag { };
typedef list_alloc<allocator_tag> alloc_type;

template <class T>
typename list_alloc<T>::data list_alloc<T>::data_;

template <typename T>
struct allocator_type : public list_allocator<T, allocator_tag> { };

namespace action_test {

  static bool up_uv_input1_flag = true;
  static bool up_uv_input2_flag = true;
  static bool up_uv_input3_flag = true;
  static bool up_v_input1_flag = true;
  static bool up_v_input2_flag = true;
  static bool up_v_input3_flag = true;
  static bool p_uv_input1_flag = true;
  static bool p_uv_input2_flag = true;
  static bool p_uv_input3_flag = true;
  static bool p_v_input1_flag = true;
  static bool p_v_input2_flag = true;
  static bool p_v_input3_flag = true;
  static bool ap_uv_input1_flag = true;
  static bool ap_uv_input2_flag = true;
  static bool ap_uv_input3_flag = true;
  static bool ap_v_input1_flag = true;
  static bool ap_v_input2_flag = true;
  static bool ap_v_input3_flag = true;

  aid_t ap_uv_input1_parameter;
  aid_t ap_uv_input2_parameter;
  aid_t ap_uv_input3_parameter;

  aid_t ap_v_input1_parameter;
  aid_t ap_v_input2_parameter;
  aid_t ap_v_input3_parameter;

  typedef fifo_scheduler<allocator_type> scheduler_type;
  static scheduler_type* scheduler_ = 0;

  static bool up_uv_output_flag = true;
  static bool up_v_output_flag = true;
  static const aid_t up_v_output_value = 101;
  static bool p_uv_output_flag = true;
  static bool p_v_output_flag = true;
  static const aid_t p_v_output_value = 104;
  static bool ap_uv_output_flag = true;
  aid_t ap_uv_output_parameter;
  static bool ap_v_output_flag = true;
  aid_t ap_v_output_parameter;
  static const aid_t ap_v_output_value = 107;

  static bool up_internal_flag = true;
  static const aid_t p_internal_parameter = 108;
  static bool p_internal_flag = true;
    
  static void
  schedule ();
  
  void
  init ()
  {
    kout << __func__ << endl;
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
    system::finish (0, 0, false, 0);
  }

  void
  up_uv_input1 ()
  {
    up_uv_input1_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  up_uv_input2 ()
  {
    up_uv_input2_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  up_uv_input3 ()
  {
    up_uv_input3_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  up_v_input1 (aid_t v)
  {
    kassert (v == up_v_output_value);
    up_v_input1_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  up_v_input2 (aid_t v)
  {
    kassert (v == p_v_output_value);
    up_v_input2_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  up_v_input3 (aid_t v)
  {
    kassert (v == ap_v_output_value);
    up_v_input3_flag = false;
    no_schedule ();
    no_finish ();
  }
  
  void
  p_uv_input1 (aid_t p)
  {
    kassert (p == p_uv_input1_parameter);
    p_uv_input1_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  p_uv_input2 (aid_t p)
  {
    kassert (p == p_uv_input2_parameter);
    p_uv_input2_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  p_uv_input3 (aid_t p)
  {
    kassert (p == p_uv_input3_parameter);
    p_uv_input3_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  p_v_input1 (aid_t p,
	      aid_t v)
  {
    kassert (p == p_v_input1_parameter);
    kassert (v == up_v_output_value);
    p_v_input1_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  p_v_input2 (aid_t p,
	      aid_t v)
  {
    kassert (p == p_v_input2_parameter);
    kassert (v == p_v_output_value);
    p_v_input2_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  p_v_input3 (aid_t p,
	      aid_t v)
  {
    kassert (p == p_v_input3_parameter);
    kassert (v == ap_v_output_value);
    p_v_input3_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  ap_uv_input1 (aid_t p)
  {
    kassert (p == ap_uv_input1_parameter);
    ap_uv_input1_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  ap_uv_input2 (aid_t p)
  {
    kassert (p == ap_uv_input2_parameter);
    ap_uv_input2_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  ap_uv_input3 (aid_t p)
  {
    kassert (p == ap_uv_input3_parameter);
    ap_uv_input3_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  ap_v_input1 (aid_t p,
	      aid_t v)
  {
    kassert (p == ap_v_input1_parameter);
    kassert (v == up_v_output_value);
    ap_v_input1_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  ap_v_input2 (aid_t p,
	       aid_t v)
  {
    kassert (p == ap_v_input2_parameter);
    kassert (v == p_v_output_value);
    ap_v_input2_flag = false;
    no_schedule ();
    no_finish ();
  }

  void
  ap_v_input3 (aid_t p,
	      aid_t v)
  {
    kassert (p == ap_v_input3_parameter);
    kassert (v == ap_v_output_value);
    ap_v_input3_flag = false;
    no_schedule ();
    no_finish ();
  }

  static bool
  up_uv_output_precondition ()
  {
    return up_uv_output_flag;
  }

  void
  up_uv_output ()
  {
    scheduler_->remove<up_uv_output_traits> (&up_uv_output);
    if (up_uv_output_precondition ()) {
      up_uv_output_flag = false;
      kout << __func__ << endl;
      schedule ();
      scheduler_->finish<up_uv_output_traits> (true);
    }
    else {
      schedule ();
      scheduler_->finish<up_uv_output_traits> (false);
    }
  }

  static bool
  up_v_output_precondition ()
  {
    return up_v_output_flag;
  }

  void
  up_v_output ()
  {
    scheduler_->remove<up_v_output_traits> (&up_v_output);
    if (up_v_output_precondition ()) {
      up_v_output_flag = false;
      static aid_t value = up_v_output_value;
      kout << __func__ << endl;
      schedule ();
      scheduler_->finish<up_v_output_traits> (&value);
    }
    else {
      schedule ();
      scheduler_->finish<up_v_output_traits> ((void*)0);
    }
  }

  static bool
  p_uv_output_precondition (aid_t p)
  {
    kassert (p == p_uv_output_parameter);
    return p_uv_output_flag;
  }

  void
  p_uv_output (aid_t p)
  {
    scheduler_->remove<p_uv_output_traits> (&p_uv_output, p);
    if (p_uv_output_precondition (p)) {
      kassert (p == p_uv_output_parameter);
      p_uv_output_flag = false;
      schedule ();
      scheduler_->finish<p_uv_output_traits> (true);
      kout << __func__ << endl;
    }
    else {
      schedule ();
      scheduler_->finish<p_uv_output_traits> (false);
    }
  }

  static bool
  p_v_output_precondition (aid_t p)
  {
    kassert (p == p_v_output_parameter);
    return p_v_output_flag;
  }

  void
  p_v_output (aid_t p)
  {
    scheduler_->remove<p_v_output_traits> (&p_v_output, p);
    if (p_v_output_precondition (p)) {
      kassert (p == p_v_output_parameter);
      p_v_output_flag = false;
      static aid_t value = p_v_output_value;
      kout << __func__ << endl;
      schedule ();
      scheduler_->finish<p_v_output_traits> (&value);
    }
    else {
      schedule ();
      scheduler_->finish<p_v_output_traits> ((void*)0);
    }
  }

  static bool
  ap_uv_output_precondition (aid_t p)
  {
    kassert (p == ap_uv_output_parameter);
    return ap_uv_output_flag;
  }

  void
  ap_uv_output (aid_t p)
  {
    scheduler_->remove<ap_uv_output_traits> (&ap_uv_output, p);
    if (ap_uv_output_precondition (p)) {
      kassert (p == ap_uv_output_parameter);
      ap_uv_output_flag = false;
      kout << __func__ << endl;
      schedule ();
      scheduler_->finish<ap_uv_output_traits> (true);
    }
    else {
      schedule ();
      scheduler_->finish<ap_uv_output_traits> (false);
    }
  }

  static bool
  ap_v_output_precondition (aid_t p)
  {
    kassert (p == ap_v_output_parameter);
    return ap_v_output_flag;
  }

  void
  ap_v_output (aid_t p)
  {
    scheduler_->remove<ap_v_output_traits> (&ap_v_output, p);
    if (ap_v_output_precondition (p)) {
      kassert (p == ap_v_output_parameter);
      ap_v_output_flag = false;
      static aid_t value = ap_v_output_value;
      kout << __func__ << endl;
      schedule ();
      scheduler_->finish<ap_v_output_traits> (&value);
    }
    else {
      schedule ();
      scheduler_->finish<ap_v_output_traits> ((void*)0);
    }
  }

  static bool
  up_internal_precondition ()
  {
    return up_internal_flag;
  }

  void
  up_internal ()
  {
    scheduler_->remove<up_internal_traits> (&up_internal);
    if (up_internal_precondition ()) {
      up_internal_flag = false;
      kout << __func__ << endl;
    }
    schedule ();
    scheduler_->finish<up_internal_traits> ();
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
      kout << __func__ << endl;
    }
    schedule ();
    scheduler_->finish<p_internal_traits> ();
  }

  static void
  schedule ()
  {
    if (up_uv_output_precondition ()) {
      scheduler_->add<up_uv_output_traits> (&up_uv_output);
    }
    if (up_v_output_precondition ()) {
      scheduler_->add<up_v_output_traits> (&up_v_output);
    }
    if (p_uv_output_precondition (p_uv_output_parameter)) {
      scheduler_->add<p_uv_output_traits> (&p_uv_output, p_uv_output_parameter);
    }
    if (p_v_output_precondition (p_v_output_parameter)) {
      scheduler_->add<p_v_output_traits> (&p_v_output, p_v_output_parameter);
    }
    if (ap_uv_output_precondition (ap_uv_output_parameter)) {
      scheduler_->add<ap_uv_output_traits> (&ap_uv_output, ap_uv_output_parameter);
    }
    if (ap_v_output_precondition (ap_v_output_parameter)) {
      scheduler_->add<ap_v_output_traits> (&ap_v_output, ap_v_output_parameter);
    }
    if (up_internal_precondition ()) {
      scheduler_->add<up_internal_traits> (&up_internal);
    }
    if (p_internal_precondition (p_internal_parameter)) {
      scheduler_->add<p_internal_traits> (&p_internal, p_internal_parameter);
    }
  }

}
