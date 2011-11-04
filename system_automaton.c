/*
  File
  ----
  system_automaton.h
  
  Description
  -----------
  The system automaton.

  Authors:
  Justin R. Wilson
*/

#include "system_automaton.h"
#include "kassert.h"
#include "descriptor.h"
#include "hash_map.h"
#include "malloc.h"
#include "scheduler.h"
#include "frame_manager.h"
#include "vm_manager.h"
#include "idt.h"
#include "string.h"

/* Macros for page faults. */
#define PAGE_FAULT_INTERRUPT 14
#define PAGE_PROTECTION_ERROR (1 << 0)
#define PAGE_WRITE_ERROR (1 << 1)
#define PAGE_USER_ERROR (1 << 2)
#define PAGE_RESERVED_ERROR (1 << 3)
#define PAGE_INSTRUCTION_ERROR (1 << 4)

/* I stole vm_area from Linux. */
typedef enum {
  VM_AREA_TEXT,
  VM_AREA_DATA,
  VM_AREA_STACK,
  VM_AREA_BUFFER,
} vm_area_type_t;

/* These need to be sorted by address. */
typedef struct vm_area vm_area_t;
struct vm_area {
  vm_area_type_t type;
  void* begin;
  void* end;
  page_privilege_t page_privilege;
  writable_t writable;
  vm_area_t* next;
};

struct automaton {
  /* Automata execute at a certain privilege level. */
  privilege_t privilege;
  /* Table of action descriptors for guiding execution, checking bindings, etc. */
  hash_map_t* actions;
  /* The scheduler uses this object. */
  void* scheduler_context;
  /* Physical address of the page directory. */
  unsigned int page_directory;
  /* Stack pointer (constant). */
  void* stack_pointer;
  /* Memory map. */
  vm_area_t* memory_map;
  /* Can't map into this area. */
  void* memory_ceiling;
};

typedef struct {
  /* aid_t aid; */
  unsigned int action_entry_point;
  unsigned int parameter;
} output_action_t;

/* Markers for the kernel. */
extern int text_begin;
extern int text_end;
extern int data_begin;
extern int data_end;
/* Stack is in data section. */
extern int stack_begin;
extern int stack_end;

/* /\* The next aid to test when creating a new automaton. *\/ */
/* static aid_t next_aid = 0; */
/* The set of automata. */
static hash_map_t* automata = 0;
/* The set of bindings. */
static hash_map_t* bindings = 0;

/* /\* The hash map of automata uses the aid as the key. *\/ */
/* static unsigned int */
/* aid_hash_func (const void* x) */
/* { */
/*   return (unsigned int)x; */
/* } */

/* static int */
/* aid_compare_func (const void* x, */
/* 		  const void* y) */
/* { */
/*   return x - y; */
/* } */

/* /\* The hash map of bindings uses the output action as the key. *\/ */
/* static unsigned int */
/* output_action_hash_func (const void* x) */
/* { */
/*   const output_action_t* ptr = x; */
/*   return ptr->aid ^ ptr->action_entry_point ^ ptr->parameter; */
/* } */

/* static int */
/* output_action_compare_func (const void* x, */
/* 			    const void* y) */
/* { */
/*   const output_action_t* p1 = x; */
/*   const output_action_t* p2 = y; */
/*   if (p1->aid != p2->aid) { */
/*     return p1->aid - p2->aid; */
/*   } */
/*   else if (p1->action_entry_point != p2->action_entry_point) { */
/*     return p1->action_entry_point - p2->action_entry_point; */
/*   } */
/*   else { */
/*     return p1->parameter - p2->parameter; */
/*   } */
/* } */

static void
page_fault_handler (registers_t* regs)
{
  /* Get the current automaton. */
  automaton_t* automaton = scheduler_get_current_automaton ();

  /* Get the faulting address. */
  void* addr;
  asm volatile ("mov %%cr2, %0\n" : "=r"(addr));
  addr = (void*)PAGE_ALIGN_DOWN (addr);

  /* Find the address in the memory map. */
  vm_area_t* ptr;
  for (ptr = automaton->memory_map; ptr != 0; ptr = ptr->next) {
    if (addr >= ptr->begin && addr < ptr->end) {
      break;
    }
  }

  if (ptr != 0) {
    if (regs->error & PAGE_PROTECTION_ERROR) {
      /* Protection error. */
      kassert (0);
    }
    else {
      /* Not present. */
      if (regs->error & PAGE_INSTRUCTION_ERROR) {
	/* Instruction. */
	kassert (0);
      }
      else {
	/* Data. */
	/* Back the request with a frame. */
	vm_manager_map (addr, frame_manager_allocate (), ptr->page_privilege, ptr->writable, PRESENT);
	/* Clear. */
	memset (addr, 0, PAGE_SIZE);
	return;
      }
    }
  }
  else {
    /* TODO:  Accessed memory not in their map. Segmentation fault. */
    kassert (0);
  }

  /* kputs ("Page fault!!\n"); */
  
  /* kputs ("Address: "); kputp (addr); kputs ("\n"); */
  /* kputs ("Codes: "); */
  /* if (regs->error & PAGE_PROTECTION_ERROR) { */
  /*   kputs ("PROTECTION "); */
  /* } */
  /* else { */
  /*   kputs ("NOT_PRESENT "); */
  /* } */

  /* if (regs->error & PAGE_WRITE_ERROR) { */
  /*   kputs ("WRITE "); */
  /* } */
  /* else { */
  /*   kputs ("READ "); */
  /* } */

  /* if (regs->error & PAGE_USER_ERROR) { */
  /*   kputs ("USER "); */
  /* } */
  /* else { */
  /*   kputs ("SUPERVISOR "); */
  /* } */

  /* if (regs->error & PAGE_RESERVED_ERROR) { */
  /*   kputs ("RESERVED "); */
  /* } */

  /* if (regs->error & PAGE_INSTRUCTION_ERROR) { */
  /*   kputs ("INSTRUCTION "); */
  /* } */
  /* else { */
  /*   kputs ("DATA "); */
  /* } */
  /* kputs ("\n"); */
  
  /* kputs ("CS: "); kputuix (regs->cs); kputs (" EIP: "); kputuix (regs->eip); kputs (" EFLAGS: "); kputuix (regs->eflags); kputs ("\n"); */
  /* kputs ("SS: "); kputuix (regs->ss); kputs (" ESP: "); kputuix (regs->useresp); kputs (" DS:"); kputuix (regs->ds); kputs ("\n"); */
  
  /* kputs ("EAX: "); kputuix (regs->eax); kputs (" EBX: "); kputuix (regs->ebx); kputs (" ECX: "); kputuix (regs->ecx); kputs (" EDX: "); kputuix (regs->edx); kputs ("\n"); */
  /* kputs ("ESP: "); kputuix (regs->esp); kputs (" EBP: "); kputuix (regs->ebp); kputs (" ESI: "); kputuix (regs->esi); kputs (" EDI: "); kputuix (regs->edi); kputs ("\n"); */
  
  /* kputs ("Halting"); */
  /* halt (); */
}

void
system_automaton_initialize (void)
{
  /* kassert (next_aid == 0); */
  kassert (automata == 0);
  kassert (bindings == 0);

  set_interrupt_handler (PAGE_FAULT_INTERRUPT, page_fault_handler);

  /* For bootstrapping, pretend that an automaton is running. */

  /* Create a simple memory map. */
  vm_area_t memory_map[3];
  /* Text. */
  memory_map[0].type = VM_AREA_TEXT;
  memory_map[0].begin = (void*)PAGE_ALIGN_DOWN (&text_begin);
  memory_map[0].end = (void*)PAGE_ALIGN_UP (&text_end);
  memory_map[0].page_privilege = SUPERVISOR;
  memory_map[0].writable = NOT_WRITABLE;
  memory_map[0].next = &memory_map[1];
  /* Data. */
  memory_map[1].type = VM_AREA_DATA;
  memory_map[1].begin = (void*)PAGE_ALIGN_DOWN (&data_begin);
  memory_map[1].end = (void*)PAGE_ALIGN_UP (frame_manager_logical_end ());
  memory_map[0].page_privilege = SUPERVISOR;
  memory_map[0].writable = WRITABLE;
  memory_map[1].next = 0;

  /* For bootstrapping, pretend that an automaton is running. */
  automaton_t fake_automaton;
  fake_automaton.privilege = RING0;
  fake_automaton.actions = 0;
  fake_automaton.scheduler_context = 0;
  fake_automaton.page_directory = vm_manager_page_directory_physical_address ();
  fake_automaton.stack_pointer = &stack_end;
  fake_automaton.memory_map = &memory_map[0];
  fake_automaton.memory_ceiling = vm_manager_page_directory_logical_address ();

  /* Initialize the scheduler so it thinks the system automaton is executing. */
  scheduler_initialize (&fake_automaton);

  /* automata = allocate_hash_map (aid_hash_func, aid_compare_func); */
  /* bindings = allocate_hash_map (output_action_hash_func, output_action_compare_func); */

  unsigned int* a = malloc (sizeof (unsigned int));
  *a = 0x12345678;
  kputuix (*a); kputs ("\n");

  kassert (0);
}

void*
automaton_allocate (automaton_t* automaton,
		    unsigned int size,
		    syserror_t* error)
{
  kassert (automaton != 0);

  if (IS_PAGE_ALIGNED (size)) {
    /* Okay.  Let's look for a data area that we can extend. */
    void* end;
    vm_area_t** ptr;
    for (ptr = &automaton->memory_map; (*ptr) != 0; ptr = &(*ptr)->next) {
      end = (*ptr)->end;
      if ((*ptr)->type == VM_AREA_DATA) {
	/* Start with the ceiling of the automaton. */
	void* ceiling = automaton->memory_ceiling;
	if ((*ptr)->next != 0) {
	  /* Ceiling drops so we don't interfere with next area. */
	  ceiling = (*ptr)->next->begin;
	}

	if (size <= (unsigned int)(ceiling - (*ptr)->end)) {
	  /* We can extend. */
	  void* retval = (*ptr)->end;
	  (*ptr)->end += size;
	  return retval;
	}
      }
    }

    if (*ptr == 0) {
      /* TODO:  Automaton doesn't end with a data area.  Try to create a new area. */
      kassert (0);
    }

    kassert (0);
  }
  else {
    *error = SYSERROR_REQUESTED_SIZE_NOT_ALIGNED;
    return 0;
  }
}

/* #include "mm.h" */
/* #include "automata.h" */
/* #include "scheduler.h" */

/* extern int producer_init_entry; */
/* extern int producer_produce_entry; */
/* extern int consumer_init_entry; */
/* extern int consumer_consume_entry; */

  /* aid_t producer = create (RING0); */
  /* set_action_type (producer, (unsigned int)&producer_init_entry, INTERNAL); */
  /* set_action_type (producer, (unsigned int)&producer_produce_entry, OUTPUT); */

  /* aid_t consumer = create (RING0); */
  /* set_action_type (consumer, (unsigned int)&consumer_init_entry, INTERNAL); */
  /* set_action_type (consumer, (unsigned int)&consumer_consume_entry, INPUT); */

  /* bind (producer, (unsigned int)&producer_produce_entry, 0, consumer, (unsigned int)&consumer_consume_entry, 0); */

  /* initialize_scheduler (); */

  /* schedule_action (producer, (unsigned int)&producer_init_entry, 0); */
  /* schedule_action (consumer, (unsigned int)&consumer_init_entry, 0); */

  /* /\* Start the scheduler.  Doesn't return. *\/ */
  /* finish_action (0, 0); */


/* static unsigned int */
/* action_entry_point_hash_func (const void* x) */
/* { */
/*   return (unsigned int)x; */
/* } */

/* static int */
/* action_entry_point_compare_func (const void* x, */
/* 				 const void* y) */
/* { */
/*   return x - y; */
/* } */

/* static automaton_t* */
/* allocate_automaton (privilege_t privilege, */
/* 		    void* scheduler_context) */
/* { */
/*   automaton_t* ptr = kmalloc (sizeof (automaton_t)); */
/*   ptr->privilege = privilege; */
/*   ptr->actions = allocate_hash_map (action_entry_point_hash_func, action_entry_point_compare_func); */
/*   ptr->scheduler_context = scheduler_context; */
/*   return ptr; */
/* } */

/* static void */
/* automaton_set_action_type (automaton_t* ptr, */
/* 			   unsigned int action_entry_point, */
/* 			   action_type_t action_type) */
/* { */
/*   kassert (ptr != 0); */
/*   kassert (!hash_map_contains (ptr->actions, (const void*)action_entry_point)); */
/*   hash_map_insert (ptr->actions, (const void*)action_entry_point, (void *)action_type); */
/* } */

/* static action_type_t */
/* automaton_get_action_type (automaton_t* ptr, */
/* 			   unsigned int action_entry_point) */
/* { */
/*   kassert (ptr != 0); */
/*   if (hash_map_contains (ptr->actions, (const void*)action_entry_point)) { */
/*     action_type_t retval = (action_type_t)hash_map_find (ptr->actions, (const void*)action_entry_point); */
/*     return retval; */
/*   } */
/*   else { */
/*     return NO_ACTION; */
/*   } */
/* } */

/* aid_t */
/* create (privilege_t privilege) */
/* { */
/*   kassert (automata != 0); */
/*   /\* Make sure that an aid is available.  We'll probably run out of memory before this happens ;) *\/ */
/*   kassert (hash_map_size (automata) !=  2147483648U); */

/*   while (hash_map_contains (automata, (const void*)next_aid)) { */
/*     ++next_aid; */
/*     if (next_aid < 0) { */
/*       next_aid = 0; */
/*     } */
/*   } */

/*   hash_map_insert (automata, (const void*)next_aid, allocate_automaton (privilege, allocate_scheduler_context (next_aid))); */

/*   return next_aid; */
/* } */

/* void* */
/* get_scheduler_context (aid_t aid) */
/* { */
/*   kassert (automata != 0); */
/*   automaton_t* ptr = hash_map_find (automata, (const void*)aid); */

/*   if (ptr != 0) { */
/*     return ptr->scheduler_context; */
/*   } */
/*   else { */
/*     return 0; */
/*   }   */
/* } */

/* void */
/* set_action_type (aid_t aid, */
/* 		 unsigned int action_entry_point, */
/* 		 action_type_t action_type) */
/* { */
/*   kassert (automata != 0); */
/*   automaton_t* ptr = hash_map_find (automata, (const void*)aid); */
/*   kassert (ptr != 0); */
/*   automaton_set_action_type (ptr, action_entry_point, action_type); */
/* } */

/* action_type_t */
/* get_action_type (aid_t aid, */
/* 		 unsigned int action_entry_point) */
/* { */
/*   kassert (automata != 0); */
/*   automaton_t* ptr = hash_map_find (automata, (const void*)aid); */
/*   kassert (ptr != 0); */
/*   return automaton_get_action_type (ptr, action_entry_point); */
/* } */

/* void */
/* switch_to_automaton (aid_t aid, */
/* 		     unsigned int action_entry_point, */
/* 		     unsigned int parameter, */
/* 		     unsigned int input_value) */
/* { */
/*   kassert (automata != 0); */
/*   automaton_t* ptr = hash_map_find (automata, (const void*)aid); */
/*   kassert (ptr != 0); */

/*   unsigned int stack_segment; */
/*   unsigned int code_segment; */

/*   switch (ptr->privilege) { */
/*   case RING0: */
/*     stack_segment = KERNEL_DATA_SELECTOR | RING0; */
/*     code_segment = KERNEL_CODE_SELECTOR | RING0; */
/*     break; */
/*   case RING1: */
/*   case RING2: */
/*     /\* These rings are not supported. *\/ */
/*     kassert (0); */
/*     break; */
/*   case RING3: */
/*     /\* Not supported yet. *\/ */
/*     kassert (0); */
/*     break; */
/*   } */

/*   asm volatile ("mov %0, %%eax\n" */
/* 		"mov %%ax, %%ss\n" */
/* 		"mov %1, %%eax\n" */
/* 		"mov %%eax, %%esp\n" */
/* 		"pushf\n" */
/* 		"pop %%eax\n" */
/* 		"or $0x200, %%eax\n" /\* Enable interupts on return. *\/ */
/* 		"push %%eax\n" */
/* 		"pushl %2\n" */
/* 		"pushl %3\n" */
/* 		"movl %4, %%ecx\n" */
/* 		"movl %5, %%edx\n" */
/* 		"iret\n" :: "r"(stack_segment), "r"(USER_STACK_LIMIT), "m"(code_segment), "m"(action_entry_point), "m"(parameter), "m"(input_value)); */
/* } */

/* static output_action_t* */
/* allocate_output_action (aid_t aid, */
/* 			unsigned int action_entry_point, */
/* 			unsigned int parameter) */
/* { */
/*   output_action_t* ptr = kmalloc (sizeof (output_action_t)); */
/*   ptr->aid = aid; */
/*   ptr->action_entry_point = action_entry_point; */
/*   ptr->parameter = parameter; */
/*   return ptr; */
/* } */

/* static input_action_t* */
/* allocate_input_action (aid_t aid, */
/* 		       unsigned int action_entry_point, */
/* 		       unsigned int parameter) */
/* { */
/*   input_action_t* ptr = kmalloc (sizeof (input_action_t)); */
/*   ptr->aid = aid; */
/*   ptr->action_entry_point = action_entry_point; */
/*   ptr->parameter = parameter; */
/*   ptr->next = 0; */
/*   return ptr; */
/* } */

/* void */
/* bind (aid_t output_aid, */
/*       unsigned int output_action_entry_point, */
/*       unsigned int output_parameter, */
/*       aid_t input_aid, */
/*       unsigned int input_action_entry_point, */
/*       unsigned int input_parameter) */
/* { */
/*   kassert (bindings != 0); */
/*   /\* TODO:  All of the bind checks. *\/ */

/*   output_action_t output_action; */
/*   output_action.aid = output_aid; */
/*   output_action.action_entry_point = output_action_entry_point; */
/*   output_action.parameter = output_parameter; */

/*   if (!hash_map_contains (bindings, &output_action)) { */
/*     hash_map_insert (bindings, allocate_output_action (output_aid, output_action_entry_point, output_parameter), 0); */
/*   } */

/*   input_action_t* input_action = allocate_input_action (input_aid, input_action_entry_point, input_parameter); */
/*   input_action->next = hash_map_find (bindings, &output_action); */
/*   hash_map_erase (bindings, &output_action); */
/*   hash_map_insert (bindings, &output_action, input_action); */
/* } */

/* input_action_t* */
/* get_bound_inputs (aid_t output_aid, */
/* 		  unsigned int output_action_entry_point, */
/* 		  unsigned int output_parameter) */
/* { */
/*   kassert (bindings != 0); */

/*   output_action_t output_action; */
/*   output_action.aid = output_aid; */
/*   output_action.action_entry_point = output_action_entry_point; */
/*   output_action.parameter = output_parameter; */
/*   return hash_map_find (bindings, &output_action); */
/* } */






/* void */
/* initialize_automata (void); */

/* /\* These functions are for manually creating automata and bindings. */
/*    They do not check for errors. */
/*    You have been warned. */
/* *\/ */

/* aid_t */
/* create (privilege_t privilege); */

/* void */
/* set_action_type (aid_t aid, */
/* 		 unsigned int action_entry_point, */
/* 		 action_type_t action_type); */

/* void */
/* bind (aid_t output_aid, */
/*       unsigned int output_action_entry_point, */
/*       unsigned int output_parameter, */
/*       aid_t input_aid, */
/*       unsigned int input_action_entry_point, */
/*       unsigned int input_parameter); */

/* /\* These functions allow the scheduler to query the set of automata and bindings. *\/ */

/* void* */
/* get_scheduler_context (aid_t aid); */

/* action_type_t */
/* get_action_type (aid_t aid, */
/* 		 unsigned int action_entry_point); */

/* void */
/* switch_to_automaton (aid_t aid, */
/* 		     unsigned int action_entry_point, */
/* 		     unsigned int parameter, */
/* 		     unsigned int input_value); */

/* input_action_t* */
/* get_bound_inputs (aid_t output_aid, */
/* 		  unsigned int output_action_entry_point, */
/* 		  unsigned int output_parameter); */
