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
#include "automaton.hpp"
#include "scheduler.hpp"
#include "elf.hpp"
#include <queue>

// Symbols to build memory maps.
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;

// Forward declaration to initialize the memory allocator.
void
initialize_allocator (void);

// Heap location for system automata.
// Starting at 4MB allow us to use the low page table of the kernel.
static const logical_address_t SYSTEM_HEAP_BEGIN = FOUR_MEGABYTES;

// The stub is an unused page-sized region above the stack used for copying pages, etc.
static const logical_address_t STUB_END = KERNEL_VIRTUAL_BASE;
static const logical_address_t STUB_BEGIN = STUB_END - PAGE_SIZE;

// The stack is right blow the stub.
static const logical_address_t STACK_END = STUB_BEGIN;
static const logical_address_t STACK_BEGIN = STACK_END - PAGE_SIZE;

// For loading the first automaton.
static bid_t automaton_bid_;
static size_t automaton_size_;
static bid_t data_bid_;
static size_t data_size_;

// The scheduler.
typedef fifo_scheduler scheduler_type;
static scheduler_type* scheduler_ = 0;

// For creating automata and allocating them an aid.
static aid_t current_aid_;
static system_automaton::aid_map_type aid_map_;

enum privilege_t {
  NORMAL,
  PRIVILEGED,
};

// Queue of create requests.
struct create_request_item {
  automaton* const parent;		// Automaton creating this automaton.
  bid_t const bid;			// Buffer containing the text and initialization data.
  size_t const automaton_offset;	// Offset and size of the automaton.
  size_t const automaton_size;
  size_t const data_offset;		// Offset and size of the automaton.
  size_t const data_size;
  privilege_t const privilege;		// Requested privilege level.

  create_request_item (automaton* p,
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
typedef std::queue<automaton*> init_queue_type;
static init_queue_type* init_queue_ = 0;

static automaton*
create_automaton (frame_t frame)
{
  // Generate an id.
  aid_t aid = current_aid_;
  while (aid_map_.find (aid) != aid_map_.end ()) {
    aid = std::max (aid + 1, 0); // Handles overflow.
  }
  current_aid_ = std::max (aid + 1, 0);
    
  // Create the automaton and insert it into the map.
  automaton* a = new (kernel_alloc ()) automaton (aid, frame);
  aid_map_.insert (std::make_pair (aid, a));
    
  // Add to the scheduler.
  scheduler::add_automaton (a);
    
  return a;
}

static void
checked_schedule (automaton* a,
		  const void* aep,
		  aid_t p = 0)
{
  automaton::const_action_iterator pos = a->action_find (aep);
  kassert (pos != a->action_end ());
  scheduler::schedule (caction (*pos, p));
}

static void
schedule ();

typedef action_traits<internal_action> first_traits;
#define FIRST_TRAITS first_traits
#define FIRST_NAME "first"
#define FIRST_DESCRIPTION ""
#define FIRST_ACTION M_INTERNAL
#define FIRST_PARAMETER M_NO_PARAMETER
extern "C" void first (no_param_t);
ACTION_DESCRIPTOR (FIRST, first);

typedef action_traits<input_action, auto_parameter> create_request_traits;
#define CREATE_REQUEST_TRAITS create_request_traits
#define CREATE_REQUEST_NAME "create_request"
#define CREATE_REQUEST_DESCRIPTION "TODO: create_request description"
#define CREATE_REQUEST_ACTION M_INPUT
#define CREATE_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void create_request (aid_t, void*, size_t, bid_t, size_t);
ACTION_DESCRIPTOR (CREATE_REQUEST, create_request);

typedef action_traits<internal_action> create_traits;
#define CREATE_TRAITS create_traits
#define CREATE_NAME "create"
#define CREATE_DESCRIPTION ""
#define CREATE_ACTION M_INTERNAL
#define CREATE_PARAMETER M_NO_PARAMETER
extern "C" void create (no_param_t);
ACTION_DESCRIPTOR (CREATE, create);

typedef action_traits<output_action, parameter<automaton*> > init_traits;
#define INIT_TRAITS init_traits
#define INIT_NAME "init"
#define INIT_DESCRIPTION "TODO: init description"
#define INIT_ACTION M_OUTPUT
#define INIT_PARAMETER M_PARAMETER
extern "C" void init (automaton*);
ACTION_DESCRIPTOR (INIT, init);

typedef action_traits<output_action, auto_parameter> create_response_traits;
#define CREATE_RESPONSE_TRAITS create_response_traits
#define CREATE_RESPONSE_NAME "create_response"
#define CREATE_RESPONSE_DESCRIPTION "TODO: create_response description"
#define CREATE_RESPONSE_ACTION M_OUTPUT
#define CREATE_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void create_response (aid_t);
ACTION_DESCRIPTOR (CREATE_RESPONSE, create_response);

typedef action_traits<input_action, auto_parameter> bind_request_traits;
#define BIND_REQUEST_TRAITS bind_request_traits
#define BIND_REQUEST_NAME "bind_request"
#define BIND_REQUEST_DESCRIPTION "TODO: bind_request description"
#define BIND_REQUEST_ACTION M_INPUT
#define BIND_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void bind_request (aid_t, void*, size_t, bid_t, size_t);
ACTION_DESCRIPTOR (BIND_REQUEST, bind_request);

typedef action_traits<internal_action> bind_traits;
#define BIND_TRAITS bind_traits
#define BIND_NAME "bind"
#define BIND_DESCRIPTION ""
#define BIND_ACTION M_INTERNAL
#define BIND_PARAMETER M_NO_PARAMETER
extern "C" void bind (no_param_t);
ACTION_DESCRIPTOR (BIND, bind);

typedef action_traits<output_action, auto_parameter> bind_response_traits;
#define BIND_RESPONSE_TRAITS bind_response_traits
#define BIND_RESPONSE_NAME "bind_response"
#define BIND_RESPONSE_DESCRIPTION "TODO: bind_response description"
#define BIND_RESPONSE_ACTION M_OUTPUT
#define BIND_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void bind_response (aid_t);
ACTION_DESCRIPTOR (BIND_RESPONSE, bind_response);

typedef action_traits<input_action, auto_parameter> loose_request_traits;
#define LOOSE_REQUEST_TRAITS loose_request_traits
#define LOOSE_REQUEST_NAME "loose_request"
#define LOOSE_REQUEST_DESCRIPTION "TODO: loose_request description"
#define LOOSE_REQUEST_ACTION M_INPUT
#define LOOSE_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void loose_request (aid_t, void*, size_t, bid_t, size_t);
ACTION_DESCRIPTOR (LOOSE_REQUEST, loose_request);

typedef action_traits<internal_action> loose_traits;
#define LOOSE_TRAITS loose_traits
#define LOOSE_NAME "loose"
#define LOOSE_DESCRIPTION ""
#define LOOSE_ACTION M_INTERNAL
#define LOOSE_PARAMETER M_NO_PARAMETER
extern "C" void loose (no_param_t);
ACTION_DESCRIPTOR (LOOSE, loose);

typedef action_traits<output_action, auto_parameter> loose_response_traits;
#define LOOSE_RESPONSE_TRAITS loose_response_traits
#define LOOSE_RESPONSE_NAME "loose_response"
#define LOOSE_RESPONSE_DESCRIPTION "TODO: loose_response description"
#define LOOSE_RESPONSE_ACTION M_OUTPUT
#define LOOSE_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void loose_response (aid_t);
ACTION_DESCRIPTOR (LOOSE_RESPONSE, loose_response);

typedef action_traits<input_action, auto_parameter> destroy_request_traits;
#define DESTROY_REQUEST_TRAITS destroy_request_traits
#define DESTROY_REQUEST_NAME "destroy_request"
#define DESTROY_REQUEST_DESCRIPTION "TODO: destroy_request description"
#define DESTROY_REQUEST_ACTION M_INPUT
#define DESTROY_REQUEST_PARAMETER M_AUTO_PARAMETER
extern "C" void destroy_request (aid_t, void*, size_t, bid_t, size_t);
ACTION_DESCRIPTOR (DESTROY_REQUEST, destroy_request);

typedef action_traits<internal_action> destroy_traits;
#define DESTROY_TRAITS destroy_traits
#define DESTROY_NAME "destroy"
#define DESTROY_DESCRIPTION ""
#define DESTROY_ACTION M_INTERNAL
#define DESTROY_PARAMETER M_NO_PARAMETER
extern "C" void destroy (no_param_t);
ACTION_DESCRIPTOR (DESTROY, destroy);

typedef action_traits<output_action, auto_parameter> destroy_response_traits;
#define DESTROY_RESPONSE_TRAITS destroy_response_traits
#define DESTROY_RESPONSE_NAME "destroy_response"
#define DESTROY_RESPONSE_DESCRIPTION "TODO: destroy_response description"
#define DESTROY_RESPONSE_ACTION M_OUTPUT
#define DESTROY_RESPONSE_PARAMETER M_AUTO_PARAMETER
extern "C" void destroy_response (aid_t);
ACTION_DESCRIPTOR (DESTROY_RESPONSE, destroy_response);

extern "C" void
first (no_param_t)
{
  // Initialize the memory allocator manually.
  initialize_allocator ();

  // Allocate a scheduler.
  scheduler_ = new scheduler_type ();
  create_request_queue_ = new create_request_queue_type ();
  init_queue_ = new init_queue_type ();

  // Create the initial automaton.
  bid_t b = lilycall::buffer_copy (automaton_bid_, 0, automaton_size_);
  const size_t data_offset = lilycall::buffer_append (b, data_bid_, 0, data_size_);
  create_request_queue_->push (create_request_item (0, b, 0, automaton_size_, data_offset, data_size_, PRIVILEGED));
  lilycall::buffer_destroy (automaton_bid_);
  lilycall::buffer_destroy (data_bid_);

  schedule ();
  scheduler_->finish ();
}

extern "C" void
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

static bool
create_precondition (void)
{
  return !create_request_queue_->empty ();
}

// The image of an automaton must be smaller than this.
// I picked this number arbitrarily.
static const size_t MAX_AUTOMATON_SIZE = 0x8000000;

extern "C" void
create (no_param_t)
{
  scheduler_->remove<create_traits> (&create);

  if (create_precondition ()) {
    const create_request_item item = create_request_queue_->front ();
    create_request_queue_->pop ();

    // Create a page directory.
    frame_t frame;
    {
      // Create a buffer and map it in.
      bid_t bid = lilycall::buffer_create (sizeof (vm::page_directory));
      kassert (bid != -1);
      vm::page_directory* pd = static_cast<vm::page_directory*> (lilycall::buffer_map (bid));
      // Force the mapping by writing to it.
      pd->entry[PAGE_ENTRY_COUNT - 1].frame_ = 1;
      // Now we can get the frame associated with the page directory.
      frame = vm::logical_address_to_frame (reinterpret_cast<logical_address_t> (pd));
      // Initialize the page directory.
      // If the second argument is vm::USER, the automaton can access the paging area, i.e., manipulate virtual memory.
      // If the second argument is vm::SUPERVISOR, the automaton cannot.
      new (pd) vm::page_directory (frame, vm::SUPERVISOR /*map_privilege*/);
      // Initialize the page directory with a copy of the kernel.
      // If the third argument is vm::USER, the automaton gains access to kernel data.
      // If the third argument is vm::SUPERVISOR, the automaton does not.
      vm::copy_page_directory (pd, vm::get_kernel_page_directory (), vm::SUPERVISOR /*kernel_privilege*/);
      
      // At this point the frame has a reference count of 3 from the following sources.
      //   buffer
      //   virtual memory
      //   page directory self reference
      
      // Destroy the buffer.
      lilycall::buffer_destroy (bid);
      
      // The frame now has a reference count of 1.
    }

    // Create the automaton.
    automaton* child = create_automaton (frame);

    // Split the buffer.
    bid_t automaton_bid = lilycall::buffer_copy (item.bid, item.automaton_offset, item.automaton_size);
    bid_t data_bid = lilycall::buffer_copy (item.bid, item.data_offset, item.data_size);

    // Get the actual buffer because we need to inspect the frames.
    const buffer* automaton_buffer = system_automaton::instance->lookup_buffer (automaton_bid);

    // Map the automaton program.  Must succeed.
    const uint8_t* a = static_cast<const uint8_t*> (lilycall::buffer_map (automaton_bid));
    kassert (a != 0);

    // Parse the automaton.
    elf::header_parser hp (a + item.automaton_offset, item.automaton_size);
    if (!hp.parse ()) {
      // TODO
      kassert (0);
    }

    // First pass:  Loadable sections.
    for (const elf::program_header_entry* e = hp.program_header_begin (); e != hp.program_header_end (); ++e) {
      if (e->type == elf::LOAD &&
	  e->memory_size != 0) {

	if (e->permissions & elf::EXECUTE) {
	  // Create an area for code.
	  vm_text_area* area = new (kernel_alloc ()) vm_text_area (e->virtual_address, e->virtual_address + e->memory_size);

	  // Assign frames.
	  for (size_t s = 0; s < e->file_size; s += PAGE_SIZE) {
	    kout << "assigning frame " << automaton_buffer->frame_at_offset (e->offset + s) << endl;
	  }
	}
	else {
	  // Data.
	  //vm_data_area* area = new (kernel_alloc ()) vm_data_area (begin, end);
	}
      }
    }

    kout << "item.bid = " << item.bid << endl;
    kout << "frame = " << frame << endl;
    kout << "child = " << hexformat (child) << endl;
    kout << "automaton_bid = " << automaton_bid << endl;
    kout << "data_bind = " << data_bid << endl;

    kassert (0);

    // Destroy the old buffer.
    lilycall::buffer_destroy (item.bid);



    // const elf::program* p = reinterpret_cast<const elf::program*> (reinterpret_cast<const uint8_t*> (h) + h->program_header_offset);
    // for (size_t idx = 0; idx < h->program_header_entry_count; ++idx, ++p) {
    //   switch (p->type) {
    //   case elf::LOAD:

    // 	kout << " offset = " << hexformat (p->offset)
    // 	     << " virtual_address = " << hexformat (p->virtual_address)
    // 	     << " file_size = " << hexformat (p->file_size)
    // 	     << " memory_size = " << hexformat (p->memory_size);

    // 	if (p->flags & elf::EXECUTE) {
    // 	  kout << " execute";
    // 	}
    // 	if (p->flags & elf::WRITE) {
    // 	  kout << " write";
    // 	}
    // 	if (p->flags & elf::READ) {
    // 	  kout << " read";
    // 	}

    // 	kout << endl;

    // 	if (p->file_size != 0 && p->offset + p->file_size > item.automaton_size) {
    // 	  // TODO:  Segment location not reported correctly.
    // 	  kassert (0);
    // 	}

    // 	if (p->alignment != PAGE_SIZE) {
    // 	  // TODO:  We only support PAGE_SIZE alignment.
    // 	  kassert (0);
    // 	}

    // 	if (p->file_size != 0 && !is_aligned (p->offset, p->alignment)) {
    // 	  // TODO:  Section is not aligned properly.
    // 	  kassert (0);
    // 	}

    // 	if (!is_aligned (p->virtual_address, p->alignment)) {
    // 	  // TODO:  Section is not aligned properly.
    // 	  kassert (0);
    // 	}

    // 	break;
    //   case elf::NOTE:
    // 	{
    // 	  if (p->offset + p->file_size > item.automaton_size) {
    // 	    // TODO:  Segment location not reported correctly.
    // 	    kassert (0);
    // 	  }

    // 	  const elf::note* note_begin = reinterpret_cast<const elf::note*> (reinterpret_cast<const uint8_t*> (h) + p->offset);
    // 	  const elf::note* note_end = reinterpret_cast<const elf::note*> (reinterpret_cast<const uint8_t*> (h) + p->offset + p->file_size);

    // 	  while (note_begin < note_end) {
    // 	    // If the next one is in range, this one is safe to process.
    // 	    if (note_begin->next () <= note_end) {
    // 	      const uint32_t* desc = static_cast<const uint32_t*> (note_begin->desc ());
    // 	      kout << "NOTE name = " << note_begin->name ()
    // 		   << " type = " << hexformat (note_begin->type)
    // 		   << " desc_size = " << note_begin->desc_size
    // 		   << " " << hexformat (*desc) << " " << hexformat (*(desc + 1)) << endl;
    // 	    }
    // 	    note_begin = note_begin->next ();
    // 	  }
    // 	}
    // 	break;
    //   }
    // }

    kassert (0);
  }

  schedule ();
  scheduler_->finish ();
}

extern "C" void
init (automaton* p)
{
  // TODO
  kassert (0);
  // scheduler_->remove<init_traits> (&init, p);
  // kassert (p == init_queue_->front ());
  // init_queue_->pop_front ();
  // schedule ();
  // scheduler_->finish<init_traits> (true);
}

extern "C" void
create_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
bind_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
bind (no_param_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
bind_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
loose_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
loose (no_param_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
loose_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
destroy_request (aid_t, void*, size_t, bid_t, size_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
destroy (no_param_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

extern "C" void
destroy_response (aid_t)
{
  // TODO
  kout << __func__ << endl;
  kassert (0);
}

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

namespace system_automaton {

  automaton* instance;

  const_automaton_iterator
  automaton_begin ()
  {
    return const_automaton_iterator (aid_map_.begin ());
  }
  
  const_automaton_iterator
  automaton_end ()
  {
    return const_automaton_iterator (aid_map_.end ());
  }

  void
  create_system_automaton (buffer* automaton_buffer,
			   size_t automaton_size,
			   buffer* data_buffer,
			   size_t data_size)
  {
    // Automaton can manipulate virtual memory.  (Could be done with buffers but this way is more direct.)
    // Automaton can access kernel data.  (That is where its code/data reside.)
    
    // Allocate a frame.
    frame_t frame = frame_manager::alloc ();
    // Map the page directory.
    vm::map (vm::get_stub1 (), frame, vm::USER, vm::WRITABLE);
    vm::page_directory* pd = reinterpret_cast<vm::page_directory*> (vm::get_stub1 ());
    // Initialize the page directory.
    // Since the second argument is vm::USER, the automaton can access the paging area, i.e., manipulate virtual memory.
    new (pd) vm::page_directory (frame, vm::USER);
    // Initialize the page directory with a copy of the kernel.
    // Since, the third argument is vm::USER, the automaton gains access to kernel data.
    vm::copy_page_directory (pd, vm::get_kernel_page_directory (), vm::USER);
    // Unmap.
    vm::unmap (vm::get_stub1 ());
    // Drop the reference count from allocation.
    size_t count = frame_manager::decref (frame);
    kassert (count == 1);

    // Create the automaton.
    // Automaton can execute privileged instructions.  (Needed when system allocator calls invlpg when mapping).
    instance = create_automaton (frame);

    // Create the memory map.
    vm_area_base* area;
    bool r;
    
    // Text.
    area = new (kernel_alloc ()) vm_text_area (reinterpret_cast<logical_address_t> (&text_begin),
					       reinterpret_cast<logical_address_t> (&text_end));
    r = instance->insert_vm_area (area);
    kassert (r);
    
    // Data.
    area = new (kernel_alloc ()) vm_data_area (reinterpret_cast<logical_address_t> (&data_begin),
					       reinterpret_cast<logical_address_t> (&data_end));
    r = instance->insert_vm_area (area);
    kassert (r);

    // Heap.
    vm_heap_area* heap_area = new (kernel_alloc ()) vm_heap_area (SYSTEM_HEAP_BEGIN);
    r = instance->insert_heap_area (heap_area);
    kassert (r);
    
    // Stack.
    vm_stack_area* stack_area = new (kernel_alloc ()) vm_stack_area (STACK_BEGIN, STACK_END);
    r = instance->insert_stack_area (stack_area);
    kassert (r);
    
    vm_stub_area* stub_area = new (kernel_alloc ()) vm_stub_area (STUB_BEGIN, STUB_END);
    r = instance->insert_stub_area (stub_area);
    kassert (r);

    // Add the actions.
    r = instance->add_action (FIRST_NAME, FIRST_DESCRIPTION, first_traits::action_type, reinterpret_cast<const void*> (&first), first_traits::parameter_mode);
    kassert (r);

    r = instance->add_action (CREATE_REQUEST_NAME, CREATE_REQUEST_DESCRIPTION, create_request_traits::action_type, reinterpret_cast<const void*> (&create_request), create_request_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (CREATE_NAME, CREATE_DESCRIPTION, create_traits::action_type, reinterpret_cast<const void*> (&create), create_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (INIT_NAME, INIT_DESCRIPTION, init_traits::action_type, reinterpret_cast<const void*> (&init), init_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (CREATE_RESPONSE_NAME, CREATE_RESPONSE_DESCRIPTION, create_response_traits::action_type, reinterpret_cast<const void*> (&create_response), create_response_traits::parameter_mode);
    kassert (r);

    r = instance->add_action (BIND_REQUEST_NAME, BIND_REQUEST_DESCRIPTION, bind_request_traits::action_type, reinterpret_cast<const void*> (&bind_request), bind_request_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (BIND_NAME, BIND_DESCRIPTION, bind_traits::action_type, reinterpret_cast<const void*> (&bind), bind_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (BIND_RESPONSE_NAME, BIND_RESPONSE_DESCRIPTION, bind_response_traits::action_type, reinterpret_cast<const void*> (&bind_response), bind_response_traits::parameter_mode);
    kassert (r);

    r = instance->add_action (LOOSE_REQUEST_NAME, LOOSE_REQUEST_DESCRIPTION, loose_request_traits::action_type, reinterpret_cast<const void*> (&loose_request), loose_request_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (LOOSE_NAME, LOOSE_DESCRIPTION, loose_traits::action_type, reinterpret_cast<const void*> (&loose), loose_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (LOOSE_RESPONSE_NAME, LOOSE_RESPONSE_DESCRIPTION, loose_response_traits::action_type, reinterpret_cast<const void*> (&loose_response), loose_response_traits::parameter_mode);
    kassert (r);

    r = instance->add_action (DESTROY_REQUEST_NAME, DESTROY_REQUEST_DESCRIPTION, destroy_request_traits::action_type, reinterpret_cast<const void*> (&destroy_request), destroy_request_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (DESTROY_NAME, DESTROY_DESCRIPTION, destroy_traits::action_type, reinterpret_cast<const void*> (&destroy), destroy_traits::parameter_mode);
    kassert (r);
    r = instance->add_action (DESTROY_RESPONSE_NAME, DESTROY_RESPONSE_DESCRIPTION, destroy_response_traits::action_type, reinterpret_cast<const void*> (&destroy_response), destroy_response_traits::parameter_mode);
    kassert (r);
    
    // Bootstrap.
    checked_schedule (instance, reinterpret_cast<const void*> (&first));

    automaton_bid_ = instance->buffer_create (*automaton_buffer);
    automaton_size_ = automaton_size;
    data_bid_ = instance->buffer_create (*data_buffer);
    data_size_ = data_size;
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
