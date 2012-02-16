#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <lily/syscall.h>
#include <lily/types.h>
#include <lily/action.h>
#include <stddef.h>
#include <stdbool.h>

/* Import the Lily namespace. */
#define FINISH_DESTROY LILY_SYSCALL_FINISH_DESTROY
#define FINISH_RETAIN LILY_SYSCALL_FINISH_RETAIN
#define FINISH_NOOP LILY_SYSCALL_FINISH_NOOP

#define INPUT LILY_ACTION_INPUT
#define OUTPUT LILY_ACTION_OUTPUT
#define INTERNAL LILY_ACTION_INTERNAL
#define SYSTEM_INPUT LILY_ACTION_SYSTEM_INPUT

#define NO_PARAMETER LILY_ACTION_NO_PARAMETER
#define PARAMETER LILY_ACTION_PARAMETER
#define AUTO_PARAMETER LILY_ACTION_AUTO_PARAMETER

#define AUTO_MAP LILY_ACTION_AUTO_MAP

#define NO_ACTION LILY_ACTION_NO_ACTION
#define INIT LILY_ACTION_INIT

/* A macro for embedding the action information. */
#define EMBED_ACTION_DESCRIPTOR(action_type, parameter_mode, flags, func_name, action_number, action_name, action_description) \
  __asm__ (".pushsection .action_info, \"\", @note\n"		   \
  ".balign 4\n"							   \
  ".long 1f - 0f\n"			/* Size of the author string. */ \
  ".long 3f - 2f\n"			/* Size of the description. */ \
  ".long 0\n"				/* The type.  0 => action descriptor. */ \
  "0: .asciz \"lily\"\n"		/* The author of the note. */ \
  "1: .balign 4\n" \
  "2:\n"				/* The description of the note. */ \
  ".long " quote (action_type) "\n"	/* Action type. */ \
  ".long " quote (parameter_mode) "\n"	/* Parameter mode. */ \
  ".long " quote (flags) "\n"		/* Flags. */ \
  ".long " #func_name "\n"		/* Action entry point. */		\
  ".long " quote (action_number) "\n"	/* Numeric identifier. */ \
  ".long 5f - 4f\n"			/* Size of the action name. */ \
  ".long 3f - 5f\n"			/* Size of the action description. */ \
  "4: .asciz \"" action_name "\"\n"		/* The action name. */ \
  "5: .asciz \"" action_description "\"\n"     /* The action description. */ \
  "3: .balign 4\n" \
       ".popsection\n");

extern int automatonerrno;

void
finish (ano_t action_number,
	int parameter,
	bd_t buffer,
	int flags);

void
exit (void);

aid_t
create (bd_t text_bd,
	size_t text_buffer_size,
	bool retain_privilege,
	bd_t data_bd);

bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      int output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      int input_parameter);

int
subscribe_destroyed (ano_t action_number,
		     aid_t aid);

bd_t
buffer_create (size_t size);

bd_t
buffer_copy (bd_t bd,
	     size_t begin,
	     size_t end);

int
buffer_destroy (bd_t bd);

size_t
buffer_size (bd_t bd);

size_t
buffer_resize (bd_t bd,
	       size_t size);

int
buffer_assign (bd_t dest,
	       size_t dest_begin,
	       bd_t src,
	       size_t src_begin,
	       size_t src_end);

size_t
buffer_append (bd_t dest,
	       bd_t src,
	       size_t begin,
	       size_t end);

void*
buffer_map (bd_t bd);

int
buffer_unmap (bd_t bd);

#define SYSCONF_PAGESIZE LILY_SYSCALL_SYSCONF_PAGESIZE

long
sysconf (int name);

size_t
pagesize (void);

#define ALIGN_UP(val, radix) (((val) + (radix) - 1) & ~((radix) - 1))

int
set_registry (aid_t aid);

aid_t
get_registry (void);

#endif /* AUTOMATON_H */
