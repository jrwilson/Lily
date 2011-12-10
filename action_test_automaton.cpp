#include "action_test_automaton.hpp"
#include "list_allocator.hpp"
#include "fifo_scheduler.hpp"
#include "kput.hpp"
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
  
  static void
  init_effect ()
  { }

  void
  init ()
  {
    // Initialize the allocator.
    alloc_type::initialize ();
    // Allocate a scheduler.
    scheduler_ = new (alloc_type ()) scheduler_type ();

    input_action <init_traits> (init_effect, schedule, scheduler_->finish ());
  }

  static void
  no_schedule () { }

  static void
  no_finish () {
    sys_finish (0, 0, false, 0);
  }

  static void
  up_uv_input1_effect ()
  {
    up_uv_input1_flag = false;
  }

  void
  up_uv_input1 ()
  {
    input_action <up_uv_input1_traits> (up_uv_input1_effect, no_schedule, no_finish);
  }

  static void
  up_uv_input2_effect ()
  {
    up_uv_input2_flag = false;
  }

  void
  up_uv_input2 ()
  {
    input_action <up_uv_input2_traits> (up_uv_input2_effect, no_schedule, no_finish);
  }

  static void
  up_uv_input3_effect ()
  {
    up_uv_input3_flag = false;
  }

  void
  up_uv_input3 ()
  {
    input_action <up_uv_input3_traits> (up_uv_input3_effect, no_schedule, no_finish);
  }

  static void
  up_v_input1_effect (aid_t& v)
  {
    kassert (v == up_v_output_value);
    up_v_input1_flag = false;
  }

  void
  up_v_input1 (aid_t v)
  {
    input_action <up_v_input1_traits> (v, up_v_input1_effect, no_schedule, no_finish);
  }

  static void
  up_v_input2_effect (aid_t& v)
  {
    kassert (v == p_v_output_value);
    up_v_input2_flag = false;
  }

  void
  up_v_input2 (aid_t v)
  {
    input_action <up_v_input2_traits> (v, up_v_input2_effect, no_schedule, no_finish);
  }

  static void
  up_v_input3_effect (aid_t& v)
  {
    kassert (v == ap_v_output_value);
    up_v_input3_flag = false;
  }

  void
  up_v_input3 (aid_t v)
  {
    input_action <up_v_input3_traits> (v, up_v_input3_effect, no_schedule, no_finish);
  }

  static void
  p_uv_input1_effect (aid_t p)
  {
    kassert (p == p_uv_input1_parameter);
    p_uv_input1_flag = false;
  }

  void
  p_uv_input1 (aid_t p)
  {
    input_action <p_uv_input1_traits> (p, p_uv_input1_effect, no_schedule, no_finish);
  }

  static void
  p_uv_input2_effect (aid_t p)
  {
    kassert (p == p_uv_input2_parameter);
    p_uv_input2_flag = false;
  }

  void
  p_uv_input2 (aid_t p)
  {
    input_action <p_uv_input2_traits> (p, p_uv_input2_effect, no_schedule, no_finish);
  }

  static void
  p_uv_input3_effect (aid_t p)
  {
    kassert (p == p_uv_input3_parameter);
    p_uv_input3_flag = false;
  }

  void
  p_uv_input3 (aid_t p)
  {
    input_action <p_uv_input3_traits> (p, p_uv_input3_effect, no_schedule, no_finish);
  }

  static void
  p_v_input1_effect (aid_t p,
		     aid_t& v)
  {
    kassert (p == p_v_input1_parameter);
    kassert (v == up_v_output_value);
    p_v_input1_flag = false;
  }

  void
  p_v_input1 (aid_t p,
	      aid_t v)
  {
    input_action <p_v_input1_traits> (p, v, p_v_input1_effect, no_schedule, no_finish);
  }

  static void
  p_v_input2_effect (aid_t p,
		     aid_t& v)
  {
    kassert (p == p_v_input2_parameter);
    kassert (v == p_v_output_value);
    p_v_input2_flag = false;
  }

  void
  p_v_input2 (aid_t p,
	      aid_t v)
  {
    input_action <p_v_input2_traits> (p, v, p_v_input2_effect, no_schedule, no_finish);
  }

  static void
  p_v_input3_effect (aid_t p,
		     aid_t& v)
  {
    kassert (p == p_v_input3_parameter);
    kassert (v == ap_v_output_value);
    p_v_input3_flag = false;
  }

  void
  p_v_input3 (aid_t p,
	      aid_t v)
  {
    input_action <p_v_input3_traits> (p, v, p_v_input3_effect, no_schedule, no_finish);
  }

  static void
  ap_uv_input1_effect (aid_t p)
  {
    kassert (p == ap_uv_input1_parameter);
    ap_uv_input1_flag = false;
  }

  void
  ap_uv_input1 (aid_t p)
  {
    input_action <ap_uv_input1_traits> (p, ap_uv_input1_effect, no_schedule, no_finish);
  }

  static void
  ap_uv_input2_effect (aid_t p)
  {
    kassert (p == ap_uv_input2_parameter);
    ap_uv_input2_flag = false;
  }

  void
  ap_uv_input2 (aid_t p)
  {
    input_action <ap_uv_input2_traits> (p, ap_uv_input2_effect, no_schedule, no_finish);
  }

  static void
  ap_uv_input3_effect (aid_t p)
  {
    kassert (p == ap_uv_input3_parameter);
    ap_uv_input3_flag = false;
  }

  void
  ap_uv_input3 (aid_t p)
  {
    input_action <ap_uv_input3_traits> (p, ap_uv_input3_effect, no_schedule, no_finish);
  }

  static void
  ap_v_input1_effect (aid_t p,
		     aid_t& v)
  {
    kassert (p == ap_v_input1_parameter);
    kassert (v == up_v_output_value);
    ap_v_input1_flag = false;
  }

  void
  ap_v_input1 (aid_t p,
	      aid_t v)
  {
    input_action <ap_v_input1_traits> (p, v, ap_v_input1_effect, no_schedule, no_finish);
  }

  static void
  ap_v_input2_effect (aid_t p,
		     aid_t& v)
  {
    kassert (p == ap_v_input2_parameter);
    kassert (v == p_v_output_value);
    ap_v_input2_flag = false;
  }

  void
  ap_v_input2 (aid_t p,
	       aid_t v)
  {
    input_action <ap_v_input2_traits> (p, v, ap_v_input2_effect, no_schedule, no_finish);
  }

  static void
  ap_v_input3_effect (aid_t p,
		     aid_t& v)
  {
    kassert (p == ap_v_input3_parameter);
    kassert (v == ap_v_output_value);
    ap_v_input3_flag = false;
  }

  void
  ap_v_input3 (aid_t p,
	      aid_t v)
  {
    input_action <ap_v_input3_traits> (p, v, ap_v_input3_effect, no_schedule, no_finish);
  }

  static bool
  up_uv_output_precondition ()
  {
    return up_uv_output_flag;
  }

  static void
  up_uv_output_effect ()
  {
    up_uv_output_flag = false;
    kputs (__func__); kputs ("\n");
  }

  void
  up_uv_output ()
  {
    output_action<up_uv_output_traits> (scheduler_->remove<up_uv_output_traits> (&up_uv_output), up_uv_output_precondition, up_uv_output_effect, schedule, scheduler_->finish ());
  }

  static bool
  up_v_output_precondition ()
  {
    return up_v_output_flag;
  }

  static void
  up_v_output_effect (aid_t* v)
  {
    up_v_output_flag = false;
    *v = up_v_output_value;
    kputs (__func__); kputs ("\n");
  }

  void
  up_v_output ()
  {
    output_action<up_v_output_traits> (scheduler_->remove<up_v_output_traits> (&up_v_output), up_v_output_precondition, up_v_output_effect, schedule, scheduler_->finish ());
  }

  static bool
  p_uv_output_precondition (aid_t p)
  {
    kassert (p == p_uv_output_parameter);
    return p_uv_output_flag;
  }

  static void
  p_uv_output_effect (aid_t p)
  {
    kassert (p == p_uv_output_parameter);
    p_uv_output_flag = false;
    kputs (__func__); kputs ("\n");
  }

  void
  p_uv_output (aid_t p)
  {
    output_action<p_uv_output_traits> (p, scheduler_->remove<p_uv_output_traits> (&p_uv_output), p_uv_output_precondition, p_uv_output_effect, schedule, scheduler_->finish ());
  }

  static bool
  p_v_output_precondition (aid_t p)
  {
    kassert (p == p_v_output_parameter);
    return p_v_output_flag;
  }

  static void
  p_v_output_effect (aid_t p,
		     aid_t* v)
  {
    kassert (p == p_v_output_parameter);
    p_v_output_flag = false;
    *v = p_v_output_value;
    kputs (__func__); kputs ("\n");
  }

  void
  p_v_output (aid_t p)
  {
    output_action<p_v_output_traits> (p, scheduler_->remove<p_v_output_traits> (&p_v_output), p_v_output_precondition, p_v_output_effect, schedule, scheduler_->finish ());
  }

  static bool
  ap_uv_output_precondition (aid_t p)
  {
    kassert (p == ap_uv_output_parameter);
    return ap_uv_output_flag;
  }

  static void
  ap_uv_output_effect (aid_t p)
  {
    kassert (p == ap_uv_output_parameter);
    ap_uv_output_flag = false;
    kputs (__func__); kputs ("\n");
  }

  void
  ap_uv_output (aid_t p)
  {
    output_action<ap_uv_output_traits> (p, scheduler_->remove<ap_uv_output_traits> (&ap_uv_output), ap_uv_output_precondition, ap_uv_output_effect, schedule, scheduler_->finish ());
  }

  static bool
  ap_v_output_precondition (aid_t p)
  {
    kassert (p == ap_v_output_parameter);
    return ap_v_output_flag;
  }

  static void
  ap_v_output_effect (aid_t p,
		      aid_t* v)
  {
    kassert (p == ap_v_output_parameter);
    ap_v_output_flag = false;
    *v = ap_v_output_value;
    kputs (__func__); kputs ("\n");
  }

  void
  ap_v_output (aid_t p)
  {
    output_action<ap_v_output_traits> (p, scheduler_->remove<ap_v_output_traits> (&ap_v_output), ap_v_output_precondition, ap_v_output_effect, schedule, scheduler_->finish ());
  }

  static bool
  up_internal_precondition ()
  {
    return up_internal_flag;
  }

  static void
  up_internal_effect ()
  {
    up_internal_flag = false;
    kputs (__func__); kputs ("\n");
  }

  void
  up_internal ()
  {
    internal_action<up_internal_traits> (scheduler_->remove<up_internal_traits> (&up_internal), up_internal_precondition, up_internal_effect, schedule, scheduler_->finish ());
  }

  static bool
  p_internal_precondition (aid_t p)
  {
    kassert (p == p_internal_parameter);
    return p_internal_flag;
  }

  static void
  p_internal_effect (aid_t p)
  {
    kassert (p == p_internal_parameter);
    p_internal_flag = false;
    kputs (__func__); kputs ("\n");
  }

  void
  p_internal (int p)
  {
    internal_action<p_internal_traits> (p, scheduler_->remove<p_internal_traits> (&p_internal), p_internal_precondition, p_internal_effect, schedule, scheduler_->finish ());
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
