#include <action_traits.hpp>
#include <lilycall.hpp>

typedef action_traits<internal_action, parameter<bid_t> > init_traits;
#define INIT_TRAITS init_traits
#define INIT_NAME "init"
#define INIT_DESCRIPTION ""
#define INIT_COMPARE M_NO_COMPARE
#define INIT_ACTION M_INTERNAL
#define INIT_PARAMETER M_PARAMETER

int x;
int y = 1;

extern "C" void
init (bid_t)
{
  x = y + 3;
  lilycall::finish (0, -1, 0, 0, -1, 0);
}
ACTION_DESCRIPTOR (INIT, init);


// /*
//   File
//   ----
//   system_automaton.cpp
  
//   Description
//   -----------
//   The system automaton.

//   Authors:
//   Justin R. Wilson
// */

// #include "system_automaton_private.hpp"
// #include "fifo_scheduler.hpp"
// #include <queue>
// #include "kassert.hpp"
// #include "rts.hpp"

// // Forward declaration to initialize the memory allocator.
// void
// initialize_allocator (void);

// // The scheduler.
// typedef fifo_scheduler scheduler_type;
// static scheduler_type* scheduler_ = 0;

// enum privilege_t {
//   NORMAL,
//   PRIVILEGED,
// };

// // Queue of create requests.
// struct create_request_item {
//   aid_t const parent;			// Automaton creating this automaton.
//   bid_t const bid;			// Buffer containing the text and initialization data.
//   size_t const automaton_offset;	// Offset and size of the automaton.
//   size_t const automaton_size;
//   size_t const data_offset;		// Offset and size of the automaton.
//   size_t const data_size;
//   privilege_t const privilege;		// Requested privilege level.

//   create_request_item (aid_t p,
// 		       bid_t b,
// 		       size_t a_o,
// 		       size_t a_s,
// 		       size_t d_o,
// 		       size_t d_s,
// 		       privilege_t priv) :
//     parent (p),
//     bid (b),
//     automaton_offset (a_o),
//     automaton_size (a_s),
//     data_offset (d_o),
//     data_size (d_s),
//     privilege (priv)
//   { }
// };
// typedef std::queue<create_request_item> create_request_queue_type;
// static create_request_queue_type* create_request_queue_ = 0;

// // Queue of automata that need to be initialized.
// struct init_item {
//   aid_t const parent;
//   aid_t const child;
//   bid_t const bid;
//   size_t const bid_size;

//   init_item (aid_t p,
// 	     aid_t c,
// 	     bid_t b,
// 	     size_t s) :
//     parent (p),
//     child (c),
//     bid (b),
//     bid_size (s)
//   { }
// };
// typedef std::queue<init_item> init_queue_type;
// static init_queue_type* init_queue_ = 0;

// // Queue of automata that have been initialized.
// struct create_response_item {
//   aid_t const parent;
//   aid_t const child;

//   create_response_item (aid_t p,
// 			aid_t c) :
//     parent (p),
//     child (c)
//   { }
// };
// typedef std::queue<create_response_item> create_response_queue_type;
// static create_response_queue_type* create_response_queue_ = 0;

// static void
// schedule ();

// // The parameter is the aid of the system automaton.
// void
// first (aid_t aid)
// {
//   // Initialize the memory allocator manually.
//   initialize_allocator ();

//   // Allocate a scheduler.
//   scheduler_ = new scheduler_type ();
//   create_request_queue_ = new create_request_queue_type ();
//   init_queue_ = new init_queue_type ();
//   create_response_queue_ = new create_response_queue_type ();

//   // Create the initial automaton.
//   bid_t b = lilycall::buffer_copy (rts::automaton_bid, 0, rts::automaton_size);
//   const size_t data_offset = lilycall::buffer_append (b, rts::data_bid, 0, rts::data_size);
//   create_request_queue_->push (create_request_item (aid, b, 0, rts::automaton_size, data_offset, rts::data_size, PRIVILEGED));
//   lilycall::buffer_destroy (rts::automaton_bid);
//   lilycall::buffer_destroy (rts::data_bid);

//   schedule ();
//   scheduler_->finish ();
// }
// ACTION_DESCRIPTOR (SA_FIRST, first);

// void
// create_request (aid_t, void*, size_t, bid_t, size_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);

//   // Automaton and data should be page aligned.
//   // if (align_up (item.automaton_size, PAGE_SIZE) + align_up (item.data_size, PAGE_SIZE) != buffer_size) {
//   //   // TODO:  Sizes are not correct.
//   //   kassert (0);
//   // }

//     // if (item.automaton_size < sizeof (elf::header) || item.automaton_size > MAX_AUTOMATON_SIZE) {
//     //   // TODO:  Automaton is too small or large, i.e., we can't map it in.
//     //   kassert (0);
//     // }
// }
// ACTION_DESCRIPTOR (SA_CREATE_REQUEST, create_request);

// static bool
// create_precondition (void)
// {
//   return !create_request_queue_->empty ();
// }

// // The image of an automaton must be smaller than this.
// // I picked this number arbitrarily.
// static const size_t MAX_AUTOMATON_SIZE = 0x8000000;

// void
// create (no_param_t)
// {
//   scheduler_->remove<system_automaton::create_traits> (&create);

//   if (create_precondition ()) {
//     const create_request_item item = create_request_queue_->front ();
//     create_request_queue_->pop ();

//     // TODO:  Error handling.

//     // Split the buffer.
//     bid_t automaton_bid = lilycall::buffer_copy (item.bid, item.automaton_offset, item.automaton_size);
//     bid_t data_bid = lilycall::buffer_copy (item.bid, item.data_offset, item.data_size);
//     lilycall::buffer_destroy (item.bid);

//     // Create the automaton.
//     aid_t aid = lilycall::create (automaton_bid, item.automaton_size);

//     // Dispense with the buffer containing the automaton text.
//     lilycall::buffer_destroy (automaton_bid);

//     // Initialize the automaton.
//     init_queue_->push (init_item (item.parent, aid, data_bid, item.data_size));
//   }

//   schedule ();
//   scheduler_->finish ();
// }
// ACTION_DESCRIPTOR (SA_CREATE, create);

// void
// init (aid_t child)
// {
//   scheduler_->remove<system_automaton::init_traits> (&init, child);
//   if (!init_queue_->empty () && init_queue_->front ().child == child) {
//     const init_item item = init_queue_->front ();
//     init_queue_->pop ();
//     create_response_queue_->push (create_response_item (item.parent, item.child));
//     schedule ();
//     scheduler_->finish (0, 0, item.bid, item.bid_size);
//   }
//   else {
//     schedule ();
//     scheduler_->finish ();
//   }
// }
// ACTION_DESCRIPTOR (SA_INIT, init);

// void
// create_response (aid_t parent)
// {
//   scheduler_->remove<system_automaton::create_response_traits> (&create_response, parent);
//   if (!create_response_queue_->empty () && create_response_queue_->front ().parent == parent) {
//     const create_response_item item = create_response_queue_->front ();
//     create_response_queue_->pop ();
//     schedule ();
//     // TODO:  Provide a useful value.
//     kout << __func__ << endl;
//     scheduler_->finish (true);
//   }
//   else {
//     schedule ();
//     scheduler_->finish ();
//   }
// }
// ACTION_DESCRIPTOR (SA_CREATE_RESPONSE, create_response);

// void
// bind_request (aid_t, void*, size_t, bid_t, size_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_BIND_REQUEST, bind_request);

// void
// bind (no_param_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_BIND, bind);

// void
// bind_response (aid_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_BIND_RESPONSE, bind_response);

// void
// loose_request (aid_t, void*, size_t, bid_t, size_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_LOOSE_REQUEST, loose_request);

// void
// loose (no_param_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_LOOSE, loose);

// void
// loose_response (aid_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_LOOSE_RESPONSE, loose_response);

// void
// destroy_request (aid_t, void*, size_t, bid_t, size_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_DESTROY_REQUEST, destroy_request);

// void
// destroy (no_param_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_DESTROY, destroy);

// void
// destroy_response (aid_t)
// {
//   // TODO
//   kout << __func__ << endl;
//   kassert (0);
// }
// ACTION_DESCRIPTOR (SA_DESTROY_RESPONSE, destroy_response);

// static void
// schedule ()
// {
//   if (create_precondition ()) {
//     scheduler_->add<system_automaton::create_traits> (&create);
//   }
//   if (!init_queue_->empty ()) {
//     scheduler_->add<system_automaton::init_traits> (&init, init_queue_->front ().child);
//   }
//   if (!create_response_queue_->empty ()) {
//     scheduler_->add<system_automaton::create_response_traits> (&create_response, create_response_queue_->front ().parent);
//   }
// }
