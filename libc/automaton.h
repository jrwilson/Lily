#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <lily/syscall.h>
#include <lily/types.h>
#include <lily/action.h>
#include <stddef.h>
#include <stdbool.h>

/* Import the Lily namespace. */
#define FINISH_NO LILY_SYSCALL_FINISH_NO
#define FINISH_YES LILY_SYSCALL_FINISH_YES
#define FINISH_DESTROY LILY_SYSCALL_FINISH_DESTROY

#define INPUT LILY_ACTION_INPUT
#define OUTPUT LILY_ACTION_OUTPUT
#define INTERNAL LILY_ACTION_INTERNAL
#define SYSTEM_INPUT LILY_ACTION_SYSTEM_INPUT

#define NO_PARAMETER LILY_ACTION_NO_PARAMETER
#define PARAMETER LILY_ACTION_PARAMETER
#define AUTO_PARAMETER LILY_ACTION_AUTO_PARAMETER

#define NO_ACTION LILY_ACTION_NO_ACTION
#define INIT LILY_ACTION_INIT
#define LOOSED LILY_ACTION_LOOSED
#define DESTROYED LILY_ACTION_DESTROYED

/* #define AUTOMATON_ESUCCESS LILY_SYSCALL_ESUCCESS */
/* #define AUTOMATON_EBADBID LILY_SYSCALL_EBADBID */
/* #define AUTOMATON_EINVAL LILY_SYSCALL_EINVAL */
/* #define AUTOMATON_ENOMEM LILY_SYSCALL_ENOMEM */

/* A macro for embedding the action information. */
#define EMBED_ACTION_DESCRIPTOR(action_type, parameter_mode, id, func_name) \
  __asm__ (".pushsection .action_info, \"\", @note\n"		   \
  ".balign 4\n"							   \
  ".long 1f - 0f\n"		/* Length of the author string. */ \
  ".long 3f - 2f\n"		/* Length of the description. */ \
  ".long 0\n"			/* The type.  0 => action descriptor. */ \
  "0: .asciz \"lily\"\n"	/* The author of the note. */ \
  "1: .balign 4\n" \
  "2:\n"			/* The description of the note. */ \
  ".long " quote (action_type) "\n"	/* Action type. */ \
  ".long " quote (parameter_mode) "\n"	/* Parameter mode. */ \
  ".long " quote (id) "\n" /* Numeric identifier. */ \
  ".long " #func_name "\n"		/* Action entry point. */		\
  "3: .balign 4\n" \
       ".popsection\n");

extern int automatonerrno;

void
finish (ano_t action_number,
	int parameter,
	bd_t buffer,
	int flags);

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
subscribe_destroyed (aid_t aid);

bd_t
buffer_create (size_t size);

bd_t
buffer_copy (bd_t bd,
	     size_t offset,
	     size_t length);

  /* inline size_t */
  /* buffer_grow (bd_t bd, */
  /* 	       size_t size) */
  /* { */
  /*   size_t off; */
  /*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_GROW), "b"(bd), "c"(size) :); */
  /*   return off; */
  /* } */

size_t
buffer_append (bd_t dest,
	       bd_t src,
	       size_t offset,
	       size_t length);

  /* inline int */
  /* buffer_assign (bd_t dest, */
  /* 		 size_t dest_offset, */
  /* 		 bd_t src, */
  /* 		 size_t src_offset, */
  /* 		 size_t length) */
  /* { */
  /*   int result; */
  /*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_ASSIGN), "b"(dest), "c"(dest_offset), "d"(src), "S"(src_offset), "D"(length) :); */
  /*   return result; */
  /* } */

void*
buffer_map (bd_t bd);

int
buffer_unmap (bd_t bd);

int
buffer_destroy (bd_t bd);

  /* inline size_t */
  /* buffer_capacity (bd_t bd) */
  /* { */
  /*   size_t size; */
  /*   asm ("int $0x81\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bd) :); */
  /*   return size; */
  /* } */

#define PAGESIZE LILY_SYSCALL_SYSCONF_PAGESIZE

long
sysconf (int name);

#endif /* AUTOMATON_H */
