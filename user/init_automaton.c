#include <io.h>
#include <stdbool.h>
#include <string.h>
#include <dymem.h>
#include <automaton.h>

#include "keyboard.h"
#include "kb_us_104.h"
#include "shell.h"
#include "terminal.h"
#include "vga.h"

typedef struct file file_t;
struct file {
  char* name;
  size_t name_size;
  bd_t buffer;
  size_t buffer_size;
  file_t* next;
};

static file_t* head = 0;

static const char*
align_up (const char* address,
	  unsigned int radix)
{
  return (const char*) (((size_t)address + radix - 1) & ~(radix - 1));
}

static unsigned int
from_hex (const char* s)
{
  unsigned int retval = 0;

  for (int idx = 0; idx < 8; ++idx) {
    retval <<= 4;
    if (s[idx] >= '0' && s[idx] <= '9') {
      retval |= s[idx] - '0';
    }
    else if (s[idx] >= 'A' && s[idx] <= 'F') {
      retval |= s[idx] - 'A' + 10;
    }
  }

  return retval;
}

typedef struct {
  char magic[6];
  char inode[8];
  char mode[8];
  char uid[8];
  char gid[8];
  char nlink[8];
  char mtime[8];
  char filesize[8];
  char dev_major[8];
  char dev_minor[8];
  char rdev_major[8];
  char rdev_minor[8];
  char namesize[8];
  char checksum[8];
} cpio_header_t;

static void
parse_cpio_header (const char* begin,
		   const char* end)
{
  if (begin >= end) {
    return;
  }

  /* Align to a 4-byte boundary. */
  begin = align_up (begin, 4);

  if (begin + sizeof (cpio_header_t) > end) {
    /* Underflow. */
    return;
  }
  cpio_header_t* h = (cpio_header_t*)begin;

  if (memcmp (h->magic, "070701", 6) == 0) {

  }
  else if (memcmp (h->magic, "070702", 6) == 0) {

  }
  else {
    /* Bad magic number. */
    return;
  }
  size_t filesize = from_hex (h->filesize);
  size_t namesize = from_hex (h->namesize);
  begin += sizeof (cpio_header_t);

  if (begin + namesize > end) {
    /* Underflow. */
    return;
  }
  if (begin[namesize - 1] != 0) {
    /* Name is not null terminated. */
    return;
  }
  const char* name = begin;
  begin += namesize;

  /* Align to a 4-byte boundary. */
  begin = align_up (begin, 4);

  if (begin + filesize > end) {
    /* Underflow. */
    return;
  }
  const char* data = begin;
  begin += filesize;

  /* Check for the trailer. */
  if (strcmp (name, "TRAILER!!!") == 0) {
    /* Done. */
    return;
  }

  /* print ("magic = "); printn (h->magic, 6); put ('\n'); */
  /* print ("inode = "); printn (h->inode, 8); put ('\n'); */
  /* print ("mode = "); printn (h->mode, 8); put ('\n'); */
  /* print ("uid = "); printn (h->uid, 8); put ('\n'); */
  /* print ("gid = "); printn (h->gid, 8); put ('\n'); */
  /* print ("nlink = "); printn (h->nlink, 8); put ('\n'); */
  /* print ("mtime = "); printn (h->mtime, 8); put ('\n'); */
  /* print ("filesize = "); printn (h->filesize, 8); put ('\n'); */
  /* print ("dev_major = "); printn (h->dev_major, 8); put ('\n'); */
  /* print ("dev_minor = "); printn (h->dev_minor, 8); put ('\n'); */
  /* print ("rdev_major = "); printn (h->rdev_major, 8); put ('\n'); */
  /* print ("rdev_minor = "); printn (h->rdev_minor, 8); put ('\n'); */
  /* print ("namesize = "); printn (h->namesize, 8); put ('\n'); */
  /* print ("checksum = "); printn (h->checksum, 8); put ('\n'); */
  /* print ("name = "); printn (name, namesize - 1); put ('\n'); */
  /* print ("data = "); printn (data, filesize); put ('\n'); */

  /* Create a new entry. */
  file_t* f = malloc (sizeof (file_t));
  /* Record the name. */
  f->name = malloc (namesize);
  memcpy (f->name, name, namesize);
  f->name_size = namesize;
  /* Create a buffer and copy the file content. */
  f->buffer = buffer_create (filesize);
  memcpy (buffer_map (f->buffer), data, filesize);
  buffer_unmap (f->buffer);
  f->buffer_size = filesize;
  f->next = head;
  head = f;

  /* Recur. */
  parse_cpio_header (begin, end);
}

void
init (int param,
      bd_t bd,
      const char* begin,
      size_t buffer_capacity)
{
  const char* end = begin + buffer_capacity;

  const char* s = "Parsing cpio archive\n";
  syslog (s, strlen (s));
  parse_cpio_header (begin, end);

  /* aid_t keyboard = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "keyboard") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     keyboard = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t kb_us_104 = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "kb_us_104") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     kb_us_104 = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t shell = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "shell") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     shell = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t terminal = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "terminal") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     terminal = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* aid_t vga = -1; */
  /* for (file_t* f = head; f != 0; f = f->next) { */
  /*   if (strcmp (f->name, "vga") == 0) { */
  /*     //print ("name = "); print (f->name); put ('\n'); */
  /*     vga = create (f->buffer, f->buffer_size, true, -1); */
  /*   } */
  /* } */

  /* bind (keyboard, KEYBOARD_SCAN_CODE, 0, kb_us_104, KB_US_104_SCAN_CODE, 0); */
  /* bind (kb_us_104, KB_US_104_STRING, 0, shell, SHELL_STDIN, 0); */
  /* bind (shell, SHELL_STDOUT, 0, terminal, TERMINAL_DISPLAY, 0); */
  /* bind (terminal, TERMINAL_VGA_OP, 0, vga, VGA_OP, 0); */

  finish (NO_ACTION, 0, bd, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

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
