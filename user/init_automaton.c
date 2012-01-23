#include <action.h>
#include <finish.h>
#include <io.h>
#include <buffer.h>

#define INIT_NAME "init"
#define INIT_DESCRIPTION ""
#define INIT_COMPARE NO_COMPARE
#define INIT_ACTION INTERNAL
#define INIT_PARAMETER PARAMETER

#define ATTRIBUTE ((0 << 12) | (0xF << 8))
#define HEIGHT 25
#define WIDTH 80

unsigned short* video_ram = (unsigned short*)0xB8000;
unsigned int x_location = 0;
unsigned int y_location = 0;

void
put (char c)
{
  // TODO:  Can we scroll with hardware?
  /* Scroll if we are at the bottom of the screen. */
  if (y_location == HEIGHT) {
    unsigned int y;
    for (y = 0; y < HEIGHT - 1; ++y) {
      for (unsigned int x = 0; x < WIDTH; ++x) {
	video_ram[y * WIDTH + x] = video_ram[(y + 1) * WIDTH + x];
      }
    }
    /* Fill last line with spaces. */
    for (unsigned int x = 0; x < WIDTH; ++x) {
      video_ram[y * WIDTH + x] = ATTRIBUTE | ' ';
    }
    --y_location;
  }
  
  switch (c) {
  case '\b':
    if (x_location > 0) {
      --x_location;
    }
    break;
  case '\t':
    /* A tab is a position divisible by 8. */
    x_location = (x_location + 8) & ~(8-1);
    break;
  case '\n':
    x_location = 0;
    ++y_location;
    break;
  case '\r':
    x_location = 0;
    break;
  default:
    /* Print the character using black on white. */
    video_ram[y_location * WIDTH + x_location] = ATTRIBUTE | c;
    /* Advance the cursor. */
    ++x_location;
    if (x_location == WIDTH) {
      ++y_location;
      x_location = 0;
    }
  }
}

void
print (const char* s)
{
  for (; *s != 0; ++s) {
    put (*s);
  }
}

void
init (bid_t buffer)
{
  // Identity map in the VGA buffer for a "Hello, world!" program.
  // The buffer starts at 0xB8000.
  // We assume an 80x25 buffer where each character requires 2 bytes: one for the character and one for the attribute (foreground and background color).
  map (video_ram, video_ram, WIDTH * HEIGHT * 2);

  // Clear the screen.
  for (size_t idx = 0; idx < 2000; ++idx) {
    video_ram[idx] = ATTRIBUTE | ' ';
  }

  const char* message = buffer_map (buffer);
  print (message);

  finish (0, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INIT, init);

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
