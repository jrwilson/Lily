/*
  File
  ----
  system_automaton.cpp
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.hpp"
#include "fifo_scheduler.hpp"
#include <queue>
#include "kassert.hpp"
#include "sacall.hpp"
#include "rts.hpp"

// Forward declaration to initialize the memory allocator.
void
initialize_allocator (void);

// The scheduler.
typedef fifo_scheduler scheduler_type;
static scheduler_type* scheduler_ = 0;

enum privilege_t {
  NORMAL,
  PRIVILEGED,
};

// Queue of create requests.
struct create_request_item {
  aid_t const parent;			// Automaton creating this automaton.
  bid_t const bid;			// Buffer containing the text and initialization data.
  size_t const automaton_offset;	// Offset and size of the automaton.
  size_t const automaton_size;
  size_t const data_offset;		// Offset and size of the automaton.
  size_t const data_size;
  privilege_t const privilege;		// Requested privilege level.

  create_request_item (aid_t p,
		       bid_t b,
		       size_t a_o,
		       size_t a_s,
		       size_t d_o,
		       size_t d_s,
		       privilege_t priv) :
    parent (p),
    bid (b),
    automaton_offset (a_o),
    automaton_size (a_s),
    data_offset (d_o),
    data_size (d_s),
    privilege (priv)
  { }
};
typedef std::queue<create_request_item> create_request_queue_type;
static create_request_queue_type* create_request_queue_ = 0;

// Queue of automata that need to be initialized.
typedef std::queue<aid_t> init_queue_type;
static init_queue_type* init_queue_ = 0;

static void
schedule ();

void
first (no_param_t)
{
  // Initialize the memory allocator manually.
  initialize_allocator ();

  // Allocate a scheduler.
  scheduler_ = new scheduler_type ();
  create_request_queue_ = new create_request_queue_type ();
  init_queue_ = new init_queue_type ();

  // Create the initial automaton.
  bid_t b = lilycall::buffer_copy (rts::automaton_bid, 0, rts::automaton_size);
  const size_t data_offset = lilycall::buffer_append (b, rts::data_bid, 0, rts::data_size);
  create_request_queue_->push (create_request_item (0, b, 0, rts::automaton_size, data_offset, rts::data_size, PRIVILEGED));
  lilycall::buffer_destroy (rts::automaton_bid);
  lilycall::buffer_destroy (rts::data_bid);

  schedule ();
  scheduler_->finish ();
}
ACTION_DESCRIPTOR (FIRST, first);

void
create_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);

  // Automaton and data should be page aligned.
  // if (align_up (item.automaton_size, PAGE_SIZE) + align_up (item.data_size, PAGE_SIZE) != buffer_size) {
  //   // TODO:  Sizes are not correct.
  //   kassert (0);
  // }

    // if (item.automaton_size < sizeof (elf::header) || item.automaton_size > MAX_AUTOMATON_SIZE) {
    //   // TODO:  Automaton is too small or large, i.e., we can't map it in.
    //   kassert (0);
    // }
}
ACTION_DESCRIPTOR (CREATE_REQUEST, create_request);

static bool
create_precondition (void)
{
  return !create_request_queue_->empty ();
}

// The image of an automaton must be smaller than this.
// I picked this number arbitrarily.
static const size_t MAX_AUTOMATON_SIZE = 0x8000000;

void
create (no_param_t)
{
  scheduler_->remove<create_traits> (&create);

  if (create_precondition ()) {
    const create_request_item item = create_request_queue_->front ();
    create_request_queue_->pop ();

    // TODO:  Error handling.

    // Split the buffer.
    bid_t automaton_bid = lilycall::buffer_copy (item.bid, item.automaton_offset, item.automaton_size);
    bid_t data_bid = lilycall::buffer_copy (item.bid, item.data_offset, item.data_size);
    lilycall::buffer_destroy (item.bid);

    sacall::create (automaton_bid, item.automaton_size);

    kout << "do something with data_bid" << endl;
    kout << "destroy automaton_bid" << endl;
    kassert (0);
  }

  schedule ();
  scheduler_->finish ();
}
ACTION_DESCRIPTOR (CREATE, create);

void
init (aid_t p)
{
  // TODO
  kassert (0);
  // scheduler_->remove<init_traits> (&init, p);
  // kassert (p == init_queue_->front ());
  // init_queue_->pop_front ();
  // schedule ();
  // scheduler_->finish<init_traits> (true);
}
ACTION_DESCRIPTOR (INIT, init);

void
create_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (CREATE_RESPONSE, create_response);

void
bind_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (BIND_REQUEST, bind_request);

void
bind (no_param_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (BIND, bind);

void
bind_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (BIND_RESPONSE, bind_response);

void
loose_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (LOOSE_REQUEST, loose_request);

void
loose (no_param_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (LOOSE, loose);

void
loose_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (LOOSE_RESPONSE, loose_response);

void
destroy_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (DESTROY_REQUEST, destroy_request);

void
destroy (no_param_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (DESTROY, destroy);

void
destroy_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}
ACTION_DESCRIPTOR (DESTROY_RESPONSE, destroy_response);

static void
schedule ()
{
  if (create_precondition ()) {
    scheduler_->add<create_traits> (&create);
  }
  if (!init_queue_->empty ()) {
    scheduler_->add<init_traits> (&init, init_queue_->front ());
  }
}

  // static void
  // bind_ (automaton* output_automaton,
  // 	 const void* output_action_entry_point,
  // 	 parameter_mode_t output_parameter_mode,
  // 	 aid_t output_parameter,
  // 	 automaton* input_automaton,
  // 	 const void* input_action_entry_point,
  // 	 parameter_mode_t input_parameter_mode,
  // 	 aid_t input_parameter,
  // 	 buffer_value_mode_t buffer_value_mode,
  // 	 copy_value_mode_t copy_value_mode,
  // 	 size_t copy_value_size,
  // 	 automaton* owner)
  // {
  //   // Check the output action dynamically.
  //   kassert (output_automaton != 0);
  //   automaton::const_action_iterator output_pos = output_automaton->action_find (output_action_entry_point);
  //   kassert (output_pos != output_automaton->action_end () &&
  // 	     (*output_pos)->type == OUTPUT &&
  // 	     (*output_pos)->parameter_mode == output_parameter_mode &&
  // 	     (*output_pos)->buffer_value_mode == buffer_value_mode &&
  // 	     (*output_pos)->copy_value_mode == copy_value_mode &&
  // 	     (*output_pos)->copy_value_size == copy_value_size);
    
  //   // Check the input action dynamically.
  //   kassert (input_automaton != 0);
  //   automaton::const_action_iterator input_pos = input_automaton->action_find (input_action_entry_point);
  //   kassert (input_pos != input_automaton->action_end () &&
  // 	     (*input_pos)->type == INPUT &&
  // 	     (*input_pos)->parameter_mode == input_parameter_mode &&
  // 	     (*input_pos)->buffer_value_mode == buffer_value_mode &&
  // 	     (*input_pos)->copy_value_mode == copy_value_mode &&
  // 	     (*input_pos)->copy_value_size == copy_value_size);
    
  //   // TODO:  All of the bind checks.
  //   caction oa (*output_pos, output_parameter);
  //   caction ia (*input_pos, input_parameter);
    
  //   output_automaton->bind_output (oa, ia, owner);
  //   input_automaton->bind_input (oa, ia, owner);
  //   owner->bind (oa, ia);
  // }

  // // TODO:  Remove all of this code.
  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (void),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (void),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>  
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (void),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::copy_value_type),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::copy_value_type),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::copy_value_type),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (bid_t),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (bid_t),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (bid_t),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (bid_t, typename InputAction::copy_value_type),
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), NO_PARAMETER, 0,
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, typename InputAction::copy_value_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, bid_t),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (typename InputAction::parameter_type, bid_t, typename InputAction::copy_value_type),
  // 		  typename InputAction::parameter_type input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), PARAMETER, aid_cast (input_parameter),
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   NO_BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  auto_parameter_tag,
  // 		  no_buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, typename InputAction::copy_value_type),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   NO_BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, bid_t),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, bid_t),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  no_copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, bid_t),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   BUFFER_VALUE, NO_COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (no_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (void),
  // 		  auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), NO_PARAMETER, 0,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (typename OutputAction::parameter_type),
  // 		  typename OutputAction::parameter_type output_parameter,
  // 		  auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), PARAMETER, aid_cast (output_parameter),
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction, class InputAction>
  // static void
  // bind_dispatch_ (auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* output_automaton,
  // 		  void (*output_ptr) (aid_t),
  // 		  aid_t output_parameter,
  // 		  auto_parameter_tag,
  // 		  buffer_value_tag,
  // 		  copy_value_tag,
  // 		  automaton* input_automaton,
  // 		  void (*input_ptr) (aid_t, bid_t, typename InputAction::copy_value_type),
  // 		  aid_t input_parameter,
  // 		  size_t copy_value_size,
  // 		  automaton* owner)
  // {
  //   bind_ (output_automaton, reinterpret_cast<const void*> (output_ptr), AUTO_PARAMETER, output_parameter,
  //    	   input_automaton, reinterpret_cast<const void*> (input_ptr), AUTO_PARAMETER, input_parameter,
  // 	   BUFFER_VALUE, COPY_VALUE, copy_value_size, owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (void),
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (void),
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (typename OutputAction::parameter_type),
  // 	typename OutputAction::parameter_type output_parameter,
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (void),
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       output_parameter,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (void),
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (IT1),
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (typename OutputAction::parameter_type),
  // 	typename OutputAction::parameter_type output_parameter,
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (IT1),
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       output_parameter,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1,
  // 	    class IT2>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (void),
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (IT1, IT2),
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1,
  // 	    class IT2>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (typename OutputAction::parameter_type),
  // 	typename OutputAction::parameter_type output_parameter,
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (IT1, IT2),
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       output_parameter,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (void),
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (typename InputAction::parameter_type),
  // 	typename InputAction::parameter_type input_parameter,
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       input_parameter,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (typename OutputAction::parameter_type),
  // 	typename OutputAction::parameter_type output_parameter,
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (typename InputAction::parameter_type),
  // 	typename InputAction::parameter_type input_parameter,
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       output_parameter,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       input_parameter,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (void),
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (typename InputAction::parameter_type, IT1),
  // 	typename InputAction::parameter_type input_parameter,
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       input_parameter,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (typename OutputAction::parameter_type),
  // 	typename OutputAction::parameter_type output_parameter,
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (typename InputAction::parameter_type, IT1),
  // 	typename InputAction::parameter_type input_parameter,
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       output_parameter,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       input_parameter,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1,
  // 	    class IT2>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (void),
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (typename InputAction::parameter_type, IT1, IT2),
  // 	typename InputAction::parameter_type input_parameter,
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       input_parameter,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }

  // template <class OutputAction,
  // 	    class InputAction,
  // 	    class IT1,
  // 	    class IT2>
  // static void
  // bind (automaton* output_automaton,
  // 	void (*output_ptr) (typename OutputAction::parameter_type),
  // 	typename OutputAction::parameter_type output_parameter,
  // 	automaton* input_automaton,
  // 	void (*input_ptr) (typename InputAction::parameter_type, IT1, IT2),
  // 	typename InputAction::parameter_type input_parameter,
  // 	automaton* owner)
  // {
  //   // Check both actions statically.
  //   STATIC_ASSERT (is_output_action<OutputAction>::value);
  //   STATIC_ASSERT (is_input_action<InputAction>::value);
  //   // Value types must be the same.    
  //   STATIC_ASSERT (std::is_same <typename OutputAction::copy_value_type COMMA typename InputAction::copy_value_type>::value);
  //   STATIC_ASSERT (std::is_same <typename OutputAction::buffer_value_type COMMA typename InputAction::buffer_value_type>::value);

  //   bind_dispatch_<OutputAction, InputAction> (typename OutputAction::parameter_category (),
  // 					       typename OutputAction::buffer_value_category (),
  // 					       typename OutputAction::copy_value_category (),
  // 					       output_automaton,
  // 					       output_ptr,
  // 					       output_parameter,
  // 					       typename InputAction::parameter_category (),
  // 					       typename InputAction::buffer_value_category (),
  // 					       typename InputAction::copy_value_category (),
  // 					       input_automaton,
  // 					       input_ptr,
  // 					       input_parameter,
  // 					       OutputAction::copy_value_size,
  // 					       owner);
  // }


  // static void
  // create_action_test_automaton ()
  // {
  //   // Create automata to test actions.
  //   {    
  //     // Create the automaton.
  //     // Can't execute privileged instructions.
  //     // Can't manipulate virtual memory.
  //     // Can access kernel code/data.
  //     input_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
      
  //     // Create the memory map.
  //     vm_area_base* area;
  //     bool r;

  //     // Text.
  //     area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
  // 						 reinterpret_cast<logical_address_t> (&text_end));
  //     r = input_automaton_->insert_vm_area (area);
  //     kassert (r);
      
  //     // Read-only data.
  //     area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
  // 						   reinterpret_cast<logical_address_t> (&rodata_end));
  //     r = input_automaton_->insert_vm_area (area);
  //     kassert (r);
      
  //     // Data.
  //     area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
  // 						 reinterpret_cast<logical_address_t> (&data_end));
  //     r = input_automaton_->insert_vm_area (area);
  //     kassert (r);
      
  //     // Heap.
  //     vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
  //     r = input_automaton_->insert_heap_area (heap_area);
  //     kassert (r);
      
  //     // Stack.
  //     vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
  //     r = input_automaton_->insert_stack_area (stack_area);
  //     kassert (r);
      
  //     // Fourth, add the actions.
  //     input_automaton_->add_action<action_test::np_nb_nc_input1_traits> (&action_test::np_nb_nc_input1);
  //     input_automaton_->add_action<action_test::np_nb_nc_input2_traits> (&action_test::np_nb_nc_input2);
  //     input_automaton_->add_action<action_test::np_nb_nc_input3_traits> (&action_test::np_nb_nc_input3);
  //     input_automaton_->add_action<action_test::np_nb_c_input1_traits> (&action_test::np_nb_c_input1);
  //     input_automaton_->add_action<action_test::np_nb_c_input2_traits> (&action_test::np_nb_c_input2);
  //     input_automaton_->add_action<action_test::np_nb_c_input3_traits> (&action_test::np_nb_c_input3);
  //     input_automaton_->add_action<action_test::np_b_nc_input1_traits> (&action_test::np_b_nc_input1);
  //     input_automaton_->add_action<action_test::np_b_nc_input2_traits> (&action_test::np_b_nc_input2);
  //     input_automaton_->add_action<action_test::np_b_nc_input3_traits> (&action_test::np_b_nc_input3);
  //     input_automaton_->add_action<action_test::np_b_c_input1_traits> (&action_test::np_b_c_input1);
  //     input_automaton_->add_action<action_test::np_b_c_input2_traits> (&action_test::np_b_c_input2);
  //     input_automaton_->add_action<action_test::np_b_c_input3_traits> (&action_test::np_b_c_input3);
  //     input_automaton_->add_action<action_test::p_nb_nc_input1_traits> (&action_test::p_nb_nc_input1);
  //     input_automaton_->add_action<action_test::p_nb_nc_input2_traits> (&action_test::p_nb_nc_input2);
  //     input_automaton_->add_action<action_test::p_nb_nc_input3_traits> (&action_test::p_nb_nc_input3);
  //     input_automaton_->add_action<action_test::p_nb_c_input1_traits> (&action_test::p_nb_c_input1);
  //     input_automaton_->add_action<action_test::p_nb_c_input2_traits> (&action_test::p_nb_c_input2);
  //     input_automaton_->add_action<action_test::p_nb_c_input3_traits> (&action_test::p_nb_c_input3);
  //     input_automaton_->add_action<action_test::p_b_nc_input1_traits> (&action_test::p_b_nc_input1);
  //     input_automaton_->add_action<action_test::p_b_nc_input2_traits> (&action_test::p_b_nc_input2);
  //     input_automaton_->add_action<action_test::p_b_nc_input3_traits> (&action_test::p_b_nc_input3);
  //     input_automaton_->add_action<action_test::p_b_c_input1_traits> (&action_test::p_b_c_input1);
  //     input_automaton_->add_action<action_test::p_b_c_input2_traits> (&action_test::p_b_c_input2);
  //     input_automaton_->add_action<action_test::p_b_c_input3_traits> (&action_test::p_b_c_input3);
  //     input_automaton_->add_action<action_test::ap_nb_nc_input1_traits> (&action_test::ap_nb_nc_input1);
  //     input_automaton_->add_action<action_test::ap_nb_nc_input2_traits> (&action_test::ap_nb_nc_input2);
  //     input_automaton_->add_action<action_test::ap_nb_nc_input3_traits> (&action_test::ap_nb_nc_input3);
  //     input_automaton_->add_action<action_test::ap_nb_c_input1_traits> (&action_test::ap_nb_c_input1);
  //     input_automaton_->add_action<action_test::ap_nb_c_input2_traits> (&action_test::ap_nb_c_input2);
  //     input_automaton_->add_action<action_test::ap_nb_c_input3_traits> (&action_test::ap_nb_c_input3);
  //     input_automaton_->add_action<action_test::ap_b_nc_input1_traits> (&action_test::ap_b_nc_input1);
  //     input_automaton_->add_action<action_test::ap_b_nc_input2_traits> (&action_test::ap_b_nc_input2);
  //     input_automaton_->add_action<action_test::ap_b_nc_input3_traits> (&action_test::ap_b_nc_input3);
  //     input_automaton_->add_action<action_test::ap_b_c_input1_traits> (&action_test::ap_b_c_input1);
  //     input_automaton_->add_action<action_test::ap_b_c_input2_traits> (&action_test::ap_b_c_input2);
  //     input_automaton_->add_action<action_test::ap_b_c_input3_traits> (&action_test::ap_b_c_input3);
  //   }

  //   {
  //     // Create the automaton.
  //     // Can't execute privileged instructions.
  //     // Can't manipulate virtual memory.
  //     // Can access kernel code/data.
  //     output_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
      
  //     // Create the automaton's memory map.
  //     vm_area_base* area;
  //     bool r;

  //     // Text.
  //     area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
  // 						 reinterpret_cast<logical_address_t> (&text_end));
  //     r = output_automaton_->insert_vm_area (area);
  //     kassert (r);
      
  //     // Read-only data.
  //     area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
  // 						   reinterpret_cast<logical_address_t> (&rodata_end));
  //     r = output_automaton_->insert_vm_area (area);
  //     kassert (r);
      
  //     // Data.
  //     area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
  // 						 reinterpret_cast<logical_address_t> (&data_end));
  //     r = output_automaton_->insert_vm_area (area);
  //     kassert (r);
      
  //     // Heap.
  //     vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
  //     r = output_automaton_->insert_heap_area (heap_area);
  //     kassert (r);
      
  //     // Stack.
  //     vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
  //     r = output_automaton_->insert_stack_area (stack_area);
  //     kassert (r);
      
  //     // Add the actions.
  //     output_automaton_->add_action<action_test::init_traits> (&action_test::init);
  //     output_automaton_->add_action<action_test::np_nb_nc_output_traits> (&action_test::np_nb_nc_output);
  //     output_automaton_->add_action<action_test::np_nb_c_output_traits> (&action_test::np_nb_c_output);
  //     output_automaton_->add_action<action_test::np_b_nc_output_traits> (&action_test::np_b_nc_output);
  //     output_automaton_->add_action<action_test::np_b_c_output_traits> (&action_test::np_b_c_output);
  //     output_automaton_->add_action<action_test::p_nb_nc_output_traits> (&action_test::p_nb_nc_output);
  //     output_automaton_->add_action<action_test::p_nb_c_output_traits> (&action_test::p_nb_c_output);
  //     output_automaton_->add_action<action_test::p_b_nc_output_traits> (&action_test::p_b_nc_output);
  //     output_automaton_->add_action<action_test::p_b_c_output_traits> (&action_test::p_b_c_output);
  //     output_automaton_->add_action<action_test::ap_nb_nc_output_traits> (&action_test::ap_nb_nc_output);
  //     output_automaton_->add_action<action_test::ap_nb_c_output_traits> (&action_test::ap_nb_c_output);
  //     output_automaton_->add_action<action_test::ap_b_nc_output_traits> (&action_test::ap_b_nc_output);
  //     output_automaton_->add_action<action_test::ap_b_c_output_traits> (&action_test::ap_b_c_output);
      
  //     output_automaton_->add_action<action_test::np_internal_traits> (&action_test::np_internal);
  //     output_automaton_->add_action<action_test::p_internal_traits> (&action_test::p_internal);

  //     // Bind.
  //     bind<system_automaton::init_traits,
  //     	action_test::init_traits> (system_automaton_, &system_automaton::init, output_automaton_,
  //     				   output_automaton_, &action_test::init, system_automaton_);
  //     init_queue_->push_back (output_automaton_);

  //     bindsys_ = true;
  //   }
  // }

  // static void
  // bind_action_test_automaton ()
  // {
  //   if (output_automaton_ != 0 && input_automaton_ != 0) {
  //     bind<action_test::np_nb_nc_output_traits,
  //     	action_test::np_nb_nc_input1_traits> (output_automaton_, &action_test::np_nb_nc_output,
  //     					      input_automaton_, &action_test::np_nb_nc_input1, system_automaton_);

  //     bind<action_test::p_nb_nc_output_traits,
  // 	action_test::np_nb_nc_input2_traits> (output_automaton_, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
  // 					      input_automaton_, &action_test::np_nb_nc_input2, system_automaton_);

  //     action_test::ap_nb_nc_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_nb_nc_output_traits,
  // 	action_test::np_nb_nc_input3_traits> (output_automaton_, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
  // 					      input_automaton_, &action_test::np_nb_nc_input3, system_automaton_);

  //     bind<action_test::np_nb_c_output_traits,
  // 	action_test::np_nb_c_input1_traits> (output_automaton_, &action_test::np_nb_c_output,
  // 					     input_automaton_, &action_test::np_nb_c_input1, system_automaton_);

  //     bind<action_test::p_nb_c_output_traits,
  // 	action_test::np_nb_c_input2_traits> (output_automaton_, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
  // 					     input_automaton_, &action_test::np_nb_c_input2, system_automaton_);

  //     action_test::ap_nb_c_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_nb_c_output_traits,
  // 	action_test::np_nb_c_input3_traits> (output_automaton_, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
  // 					     input_automaton_, &action_test::np_nb_c_input3, system_automaton_);

  //     bind<action_test::np_b_nc_output_traits,
  // 	action_test::np_b_nc_input1_traits> (output_automaton_, &action_test::np_b_nc_output,
  // 					     input_automaton_, &action_test::np_b_nc_input1, system_automaton_);
      
  //     bind<action_test::p_b_nc_output_traits,
  // 	action_test::np_b_nc_input2_traits> (output_automaton_, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
  // 					     input_automaton_, &action_test::np_b_nc_input2, system_automaton_);
      
  //     action_test::ap_b_nc_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_b_nc_output_traits,
  // 	action_test::np_b_nc_input3_traits> (output_automaton_, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
  // 					     input_automaton_, &action_test::np_b_nc_input3, system_automaton_);
      
  //     bind<action_test::np_b_c_output_traits,
  // 	action_test::np_b_c_input1_traits> (output_automaton_, &action_test::np_b_c_output,
  // 					    input_automaton_, &action_test::np_b_c_input1, system_automaton_);
      
  //     bind<action_test::p_b_c_output_traits,
  // 	action_test::np_b_c_input2_traits> (output_automaton_, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
  // 					    input_automaton_, &action_test::np_b_c_input2, system_automaton_);
      
  //     action_test::ap_b_c_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_b_c_output_traits,
  // 	action_test::np_b_c_input3_traits> (output_automaton_, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
  // 					    input_automaton_, &action_test::np_b_c_input3, system_automaton_);
      
  //     bind<action_test::np_nb_nc_output_traits,
  // 	action_test::p_nb_nc_input1_traits> (output_automaton_, &action_test::np_nb_nc_output,
  // 					     input_automaton_, &action_test::p_nb_nc_input1, action_test::p_nb_nc_input1_parameter, system_automaton_);
      
  //     bind<action_test::p_nb_nc_output_traits,
  // 	action_test::p_nb_nc_input2_traits> (output_automaton_, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
  // 					     input_automaton_, &action_test::p_nb_nc_input2, action_test::p_nb_nc_input2_parameter, system_automaton_);
      
  //     action_test::ap_nb_nc_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_nb_nc_output_traits,
  // 	action_test::p_nb_nc_input3_traits> (output_automaton_, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
  // 					     input_automaton_, &action_test::p_nb_nc_input3, action_test::p_nb_nc_input3_parameter, system_automaton_);
      
  //     bind<action_test::np_nb_c_output_traits,
  // 	action_test::p_nb_c_input1_traits> (output_automaton_, &action_test::np_nb_c_output,
  // 					    input_automaton_, &action_test::p_nb_c_input1, action_test::p_nb_c_input1_parameter, system_automaton_);
      
  //     bind<action_test::p_nb_c_output_traits,
  // 	action_test::p_nb_c_input2_traits> (output_automaton_, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
  // 					    input_automaton_, &action_test::p_nb_c_input2, action_test::p_nb_c_input2_parameter, system_automaton_);
      
  //     action_test::ap_nb_c_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_nb_c_output_traits,
  // 	action_test::p_nb_c_input3_traits> (output_automaton_, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
  // 					    input_automaton_, &action_test::p_nb_c_input3, action_test::p_nb_c_input3_parameter, system_automaton_);
      
  //     bind<action_test::np_b_nc_output_traits,
  // 	action_test::p_b_nc_input1_traits> (output_automaton_, &action_test::np_b_nc_output,
  // 					    input_automaton_, &action_test::p_b_nc_input1, action_test::p_b_nc_input1_parameter, system_automaton_);
      
  //     bind<action_test::p_b_nc_output_traits,
  // 	action_test::p_b_nc_input2_traits> (output_automaton_, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
  // 					    input_automaton_, &action_test::p_b_nc_input2, action_test::p_b_nc_input2_parameter, system_automaton_);
      
  //     action_test::ap_b_nc_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_b_nc_output_traits,
  // 	action_test::p_b_nc_input3_traits> (output_automaton_, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
  // 					    input_automaton_, &action_test::p_b_nc_input3, action_test::p_b_nc_input3_parameter, system_automaton_);
      
  //     bind<action_test::np_b_c_output_traits,
  // 	action_test::p_b_c_input1_traits> (output_automaton_, &action_test::np_b_c_output,
  // 					   input_automaton_, &action_test::p_b_c_input1, action_test::p_b_c_input1_parameter, system_automaton_);
      
  //     bind<action_test::p_b_c_output_traits,
  // 	action_test::p_b_c_input2_traits> (output_automaton_, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
  // 					   input_automaton_, &action_test::p_b_c_input2, action_test::p_b_c_input2_parameter, system_automaton_);
      
  //     action_test::ap_b_c_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_b_c_output_traits,
  // 	action_test::p_b_c_input3_traits> (output_automaton_, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
  // 					   input_automaton_, &action_test::p_b_c_input3, action_test::p_b_c_input3_parameter, system_automaton_);
      
  //     action_test::ap_nb_nc_input1_parameter = output_automaton_->aid ();
  //     bind<action_test::np_nb_nc_output_traits,
  // 	action_test::ap_nb_nc_input1_traits> (output_automaton_, &action_test::np_nb_nc_output,
  // 					      input_automaton_, &action_test::ap_nb_nc_input1, action_test::ap_nb_nc_input1_parameter, system_automaton_);
      
  //     action_test::ap_nb_nc_input2_parameter = output_automaton_->aid ();
  //     bind<action_test::p_nb_nc_output_traits,
  // 	action_test::ap_nb_nc_input2_traits> (output_automaton_, &action_test::p_nb_nc_output, action_test::p_nb_nc_output_parameter,
  // 					      input_automaton_, &action_test::ap_nb_nc_input2, action_test::ap_nb_nc_input2_parameter, system_automaton_);
      
  //     action_test::ap_nb_nc_input3_parameter = output_automaton_->aid ();
  //     action_test::ap_nb_nc_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_nb_nc_output_traits,
  // 	action_test::ap_nb_nc_input3_traits> (output_automaton_, &action_test::ap_nb_nc_output, action_test::ap_nb_nc_output_parameter,
  // 					      input_automaton_, &action_test::ap_nb_nc_input3, action_test::ap_nb_nc_input3_parameter, system_automaton_);
      
  //     action_test::ap_nb_c_input1_parameter = output_automaton_->aid ();
  //     bind<action_test::np_nb_c_output_traits,
  // 	action_test::ap_nb_c_input1_traits> (output_automaton_, &action_test::np_nb_c_output,
  // 					     input_automaton_, &action_test::ap_nb_c_input1, action_test::ap_nb_c_input1_parameter, system_automaton_);
      
  //     action_test::ap_nb_c_input2_parameter = output_automaton_->aid ();
  //     bind<action_test::p_nb_c_output_traits,
  // 	action_test::ap_nb_c_input2_traits> (output_automaton_, &action_test::p_nb_c_output, action_test::p_nb_c_output_parameter,
  // 					     input_automaton_, &action_test::ap_nb_c_input2, action_test::ap_nb_c_input2_parameter, system_automaton_);
      
  //     action_test::ap_nb_c_input3_parameter = output_automaton_->aid ();
  //     action_test::ap_nb_c_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_nb_c_output_traits,
  // 	action_test::ap_nb_c_input3_traits> (output_automaton_, &action_test::ap_nb_c_output, action_test::ap_nb_c_output_parameter,
  // 					     input_automaton_, &action_test::ap_nb_c_input3, action_test::ap_nb_c_input3_parameter, system_automaton_);
      
  //     action_test::ap_b_nc_input1_parameter = output_automaton_->aid ();
  //     bind<action_test::np_b_nc_output_traits,
  // 	action_test::ap_b_nc_input1_traits> (output_automaton_, &action_test::np_b_nc_output,
  // 					     input_automaton_, &action_test::ap_b_nc_input1, action_test::ap_b_nc_input1_parameter, system_automaton_);
      
  //     action_test::ap_b_nc_input2_parameter = output_automaton_->aid ();
  //     bind<action_test::p_b_nc_output_traits,
  // 	action_test::ap_b_nc_input2_traits> (output_automaton_, &action_test::p_b_nc_output, action_test::p_b_nc_output_parameter,
  // 					     input_automaton_, &action_test::ap_b_nc_input2, action_test::ap_b_nc_input2_parameter, system_automaton_);
      
  //     action_test::ap_b_nc_input3_parameter = output_automaton_->aid ();
  //     action_test::ap_b_nc_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_b_nc_output_traits,
  // 	action_test::ap_b_nc_input3_traits> (output_automaton_, &action_test::ap_b_nc_output, action_test::ap_b_nc_output_parameter,
  // 					     input_automaton_, &action_test::ap_b_nc_input3, action_test::ap_b_nc_input3_parameter, system_automaton_);
    
  //     action_test::ap_b_c_input1_parameter = output_automaton_->aid ();
  //     bind<action_test::np_b_c_output_traits,
  // 	action_test::ap_b_c_input1_traits> (output_automaton_, &action_test::np_b_c_output,
  // 					    input_automaton_, &action_test::ap_b_c_input1, action_test::ap_b_c_input1_parameter, system_automaton_);
    
  //     action_test::ap_b_c_input2_parameter = output_automaton_->aid ();
  //     bind<action_test::p_b_c_output_traits,
  // 	action_test::ap_b_c_input2_traits> (output_automaton_, &action_test::p_b_c_output, action_test::p_b_c_output_parameter,
  // 					    input_automaton_, &action_test::ap_b_c_input2, action_test::ap_b_c_input2_parameter, system_automaton_);
    
  //     action_test::ap_b_c_input3_parameter = output_automaton_->aid ();
  //     action_test::ap_b_c_output_parameter = input_automaton_->aid ();
  //     bind<action_test::ap_b_c_output_traits,
  // 	action_test::ap_b_c_input3_traits> (output_automaton_, &action_test::ap_b_c_output, action_test::ap_b_c_output_parameter,
  // 					    input_automaton_, &action_test::ap_b_c_input3, action_test::ap_b_c_input3_parameter, system_automaton_);

  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_nb_nc_output));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_nb_nc_output), aid_cast (action_test::p_nb_nc_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_nb_nc_output), aid_cast (action_test::ap_nb_nc_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_nb_c_output));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_nb_c_output), aid_cast (action_test::p_nb_c_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_nb_c_output), aid_cast (action_test::ap_nb_c_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_b_nc_output));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_b_nc_output), aid_cast (action_test::p_b_nc_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_b_nc_output), aid_cast (action_test::ap_b_nc_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::np_b_c_output));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::p_b_c_output), aid_cast (action_test::p_b_c_output_parameter));
  //     checked_schedule (output_automaton_, reinterpret_cast<const void*> (&action_test::ap_b_c_output), aid_cast (action_test::ap_b_c_output_parameter));
  //   }
  // }

  // static void
  // create_buffer_test_automaton ()
  // {
  //   // Create the automaton.
  //   // Can't execute privileged instructions.
  //   // Can't manipulate virtual memory.
  //   // Can access kernel code/data.
  //   buffer_test_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
  //   // Create the memory map.
  //   vm_area_base* area;
  //   bool r;
    
  //   // Text.
  //   area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
  // 					       reinterpret_cast<logical_address_t> (&text_end));
  //   r = buffer_test_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Read-only data.
  //   area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
  // 						 reinterpret_cast<logical_address_t> (&rodata_end));
  //   r = buffer_test_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Data.
  //   area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
  // 						 reinterpret_cast<logical_address_t> (&data_end));
  //   r = buffer_test_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Heap.
  //   vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
  //   r = buffer_test_automaton_->insert_heap_area (heap_area);
  //   kassert (r);
    
  //   // Stack.
  //   vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
  //   r = buffer_test_automaton_->insert_stack_area (stack_area);
  //   kassert (r);
      
  //   // Add the actions.
  //   buffer_test_automaton_->add_action<buffer_test::init_traits> (&buffer_test::init);

  //   // Bind.
  //   bind<system_automaton::init_traits,
  //     buffer_test::init_traits> (system_automaton_, &system_automaton::init, buffer_test_automaton_,
  // 				 buffer_test_automaton_, &buffer_test::init, system_automaton_);
  //   init_queue_->push_back (buffer_test_automaton_);
  //   bindsys_ = true;
  // }

  // static void
  // bind_buffer_test_automaton ()
  // {
  //   if (buffer_test_automaton_ != 0) {

  //   }
  // }

  // static void
  // create_ramdisk_automaton ()
  // {
  //   // Create the automaton.
  //   // Can't execute privileged instructions.
  //   // Can't manipulate virtual memory.
  //   // Can access kernel code/data.
  //   ramdisk_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
  //   // Create the memory map.
  //   vm_area_base* area;
  //   bool r;
    
  //   // Text.
  //   area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
  // 					       reinterpret_cast<logical_address_t> (&text_end));
  //   r = ramdisk_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Read-only data.
  //   area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
  // 						 reinterpret_cast<logical_address_t> (&rodata_end));
  //   r = ramdisk_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Data.
  //   area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
  // 						 reinterpret_cast<logical_address_t> (&data_end));
  //   r = ramdisk_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Heap.
  //   vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
  //   r = ramdisk_automaton_->insert_heap_area (heap_area);
  //   kassert (r);
    
  //   // Stack.
  //   vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
  //   r = ramdisk_automaton_->insert_stack_area (stack_area);
  //   kassert (r);

  //   // Add the actions.
  //   ramdisk_automaton_->add_action<ramdisk::init_traits> (&ramdisk::init);
  //   ramdisk_automaton_->add_action<ramdisk::info_request_traits> (&ramdisk::info_request);
  //   ramdisk_automaton_->add_action<ramdisk::info_response_traits> (&ramdisk::info_response);
  //   ramdisk_automaton_->add_action<ramdisk::read_request_traits> (&ramdisk::read_request);
  //   ramdisk_automaton_->add_action<ramdisk::read_response_traits> (&ramdisk::read_response);
  //   ramdisk_automaton_->add_action<ramdisk::write_request_traits> (&ramdisk::write_request);
  //   ramdisk_automaton_->add_action<ramdisk::write_response_traits> (&ramdisk::write_response);
    
  //   // Bind.
  //   bind<system_automaton::init_traits,
  //     ramdisk::init_traits> (system_automaton_, &system_automaton::init, ramdisk_automaton_,
  // 			     ramdisk_automaton_, &ramdisk::init, system_automaton_);
  //   init_queue_->push_back (ramdisk_automaton_);
    
  //   buffer* b = new (system_alloc ()) buffer (physical_address_to_frame (initrd_begin_), physical_address_to_frame (align_up (initrd_end_, PAGE_SIZE)));
  //   ramdisk::bid = ramdisk_automaton_->buffer_create (*b);
  //   destroy (b, system_alloc ());
  //   bindsys_ = true;
  // }

  // static void
  // bind_ramdisk_automaton ()
  // {
  //   if (ramdisk_automaton_ != 0) {

  //   }
  // }

  // static void
  // create_ext2_automaton ()
  // {
  //   // Create the automaton.
  //   // Can't execute privileged instructions.
  //   // Can't manipulate virtual memory.
  //   // Can access kernel code/data.
  //   ext2_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
  //   // Create the memory map.
  //   vm_area_base* area;
  //   bool r;
    
  //   // Text.
  //   area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
  //   					       reinterpret_cast<logical_address_t> (&text_end));
  //   r = ext2_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Read-only data.
  //   area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
  //   						 reinterpret_cast<logical_address_t> (&rodata_end));
  //   r = ext2_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Data.
  //   area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
  //   						 reinterpret_cast<logical_address_t> (&data_end));
  //   r = ext2_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Heap.
  //   vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
  //   r = ext2_automaton_->insert_heap_area (heap_area);
  //   kassert (r);
    
  //   // Stack.
  //   vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
  //   r = ext2_automaton_->insert_stack_area (stack_area);
  //   kassert (r);

  //   // Add the actions.
  //   ext2_automaton_->add_action<ext2::init_traits> (&ext2::init);
  //   ext2_automaton_->add_action<ext2::info_request_traits> (&ext2::info_request);
  //   ext2_automaton_->add_action<ext2::info_response_traits> (&ext2::info_response);
  //   ext2_automaton_->add_action<ext2::read_request_traits> (&ext2::read_request);
  //   ext2_automaton_->add_action<ext2::read_response_traits> (&ext2::read_response);
  //   ext2_automaton_->add_action<ext2::write_request_traits> (&ext2::write_request);
  //   ext2_automaton_->add_action<ext2::write_response_traits> (&ext2::write_response);
  //   ext2_automaton_->add_action<ext2::generate_read_request_traits> (&ext2::generate_read_request);
  //   ext2_automaton_->add_action<ext2::generate_write_request_traits> (&ext2::generate_write_request);

  //   // Bind.
  //   bind<system_automaton::init_traits,
  //     ext2::init_traits> (system_automaton_, &system_automaton::init, ext2_automaton_,
  //   			  ext2_automaton_, &ext2::init, system_automaton_);
  //   init_queue_->push_back (ext2_automaton_);
  //   bindsys_ = true;
  // }

  // static void
  // bind_ext2_automaton ()
  // {
  //   if (ext2_automaton_ != 0) {
  //     bind<ext2::info_request_traits,
  //       ramdisk::info_request_traits> (ext2_automaton_, &ext2::info_request,
  // 				       ramdisk_automaton_, &ramdisk::info_request, ext2_automaton_->aid (), system_automaton_);
  //     checked_schedule (ext2_automaton_, reinterpret_cast<const void*> (&ext2::info_request));
      
  //     bind<ramdisk::info_response_traits,
  //       ext2::info_response_traits> (ramdisk_automaton_, &ramdisk::info_response, ext2_automaton_->aid (),
  // 				     ext2_automaton_, &ext2::info_response, system_automaton_);
  //     checked_schedule (ramdisk_automaton_, reinterpret_cast<const void*> (&ramdisk::info_response), ext2_automaton_->aid ());

  //     bind<ext2::read_request_traits,
  //       ramdisk::read_request_traits> (ext2_automaton_, &ext2::read_request,
  // 				       ramdisk_automaton_, &ramdisk::read_request, ext2_automaton_->aid (), system_automaton_);
  //     checked_schedule (ext2_automaton_, reinterpret_cast<const void*> (&ext2::read_request));
      
  //     bind<ramdisk::read_response_traits,
  //       ext2::read_response_traits> (ramdisk_automaton_, &ramdisk::read_response, ext2_automaton_->aid (),
  // 				     ext2_automaton_, &ext2::read_response, system_automaton_);
  //     checked_schedule (ramdisk_automaton_, reinterpret_cast<const void*> (&ramdisk::read_response), ext2_automaton_->aid ());

  //     bind<ext2::write_request_traits,
  //       ramdisk::write_request_traits> (ext2_automaton_, &ext2::write_request,
  // 				       ramdisk_automaton_, &ramdisk::write_request, ext2_automaton_->aid (), system_automaton_);
  //     checked_schedule (ext2_automaton_, reinterpret_cast<const void*> (&ext2::write_request));
      
  //     bind<ramdisk::write_response_traits,
  //       ext2::write_response_traits> (ramdisk_automaton_, &ramdisk::write_response, ext2_automaton_->aid (),
  // 				     ext2_automaton_, &ext2::write_response, system_automaton_);
  //     checked_schedule (ramdisk_automaton_, reinterpret_cast<const void*> (&ramdisk::write_response), ext2_automaton_->aid ());
  //   }
  // }

  // static void
  // create_boot_automaton ()
  // {
  //   // Create the automaton.
  //   // Can't execute privileged instructions.
  //   // Can't manipulate virtual memory.
  //   // Can access kernel code/data.
  //   boot_automaton_ = create (false, vm::SUPERVISOR, vm::USER);
    
  //   // Create the memory map.
  //   vm_area_base* area;
  //   bool r;
    
  //   // Text.
  //   area = new (system_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
  //   					       reinterpret_cast<logical_address_t> (&text_end));
  //   r = boot_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Read-only data.
  //   area = new (system_alloc ()) vm_rodata_area (reinterpret_cast<logical_address_t> (&rodata_begin),
  //   						 reinterpret_cast<logical_address_t> (&rodata_end));
  //   r = boot_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Data.
  //   area = new (system_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
  //   						 reinterpret_cast<logical_address_t> (&data_end));
  //   r = boot_automaton_->insert_vm_area (area);
  //   kassert (r);
    
  //   // Heap.
  //   vm_heap_area* heap_area = new (system_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
  //   r = boot_automaton_->insert_heap_area (heap_area);
  //   kassert (r);
    
  //   // Stack.
  //   vm_stack_area* stack_area = new (system_alloc ()) vm_stack_area (SYSTEM_STACK_BEGIN, SYSTEM_STACK_END);
  //   r = boot_automaton_->insert_stack_area (stack_area);
  //   kassert (r);

  //   // Add the actions.
  //   boot_automaton_->add_action<boot::init_traits> (&boot::init);
  //   boot_automaton_->add_action<boot::create_request_traits> (&boot::create_request);
  //   boot_automaton_->add_action<boot::create_response_traits> (&boot::create_response);

  //   // Bind.
  //   bind<system_automaton::init_traits,
  //     boot::init_traits> (system_automaton_, &system_automaton::init, boot_automaton_,
  //   			  boot_automaton_, &boot::init, system_automaton_);
  //   bind<boot::create_request_traits,
  //     system_automaton::create_request_traits> (boot_automaton_, &boot::create_request,
  // 						system_automaton_, &system_automaton::create_request, boot_automaton_->aid (), system_automaton_);
  //   bind<system_automaton::create_response_traits,
  //     boot::create_response_traits> (system_automaton_, &system_automaton::create_response, boot_automaton_->aid (),
  // 				     boot_automaton_, &boot::create_response, system_automaton_);

  //   init_queue_->push_back (boot_automaton_);
  //   bindsys_ = true;
  // }

  // static void
  // bind_boot_automaton ()
  // {
  //   if (boot_automaton_ != 0) {

  //   }
  // }
