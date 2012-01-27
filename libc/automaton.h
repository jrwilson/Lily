#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <lily/syscall.h>
#include <lily/types.h>
#include <lily/action.h>
#include <stddef.h>
#include <stdbool.h>

/* Import the Lily namespace. */

#define INPUT LILY_ACTION_INPUT
#define OUTPUT LILY_ACTION_OUTPUT
#define INTERNAL LILY_ACTION_INTERNAL

#define NO_PARAMETER LILY_ACTION_NO_PARAMETER
#define PARAMETER LILY_ACTION_PARAMETER
#define AUTO_PARAMETER LILY_ACTION_AUTO_PARAMETER

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
	const void* parameter,
	const void* value,
	size_t value_size,
	bd_t buffer,
	size_t buffer_size);

size_t
binding_count (ano_t action_number,
	       const void* parameter);

aid_t
create (bd_t buffer,
	size_t buffer_size,
	bool retain_privilege);

bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      const void* output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      const void* input_parameter);

extern int buffererrno;

bd_t
buffer_create (size_t size);

  /* inline bd_t */
  /* buffer_copy (bd_t b, */
  /* 	       size_t offset, */
  /* 	       size_t length) */
  /* { */
  /*   bd_t bd; */
  /*   asm ("int $0x81\n" : "=a"(bd) : "0"(BUFFER_COPY), "b"(b), "c"(offset), "d"(length) :); */
  /*   return bd; */
  /* } */

  /* inline size_t */
  /* buffer_grow (bd_t bd, */
  /* 	       size_t size) */
  /* { */
  /*   size_t off; */
  /*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_GROW), "b"(bd), "c"(size) :); */
  /*   return off; */
  /* } */

  /* inline size_t */
  /* buffer_append (bd_t dest, */
  /* 		 bd_t src, */
  /* 		 size_t offset, */
  /* 		 size_t length) */
  /* { */
  /*   size_t off; */
  /*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_APPEND), "b"(dest), "c"(src), "d"(offset), "S"(length) :); */
  /*   return off; */
  /* } */

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
  /* buffer_size (bd_t bd) */
  /* { */
  /*   size_t size; */
  /*   asm ("int $0x81\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bd) :); */
  /*   return size; */
  /* } */

#define PAGESIZE LILY_SYSCALL_SYSCONF_PAGESIZE

long
sysconf (int name);

#endif /* AUTOMATON_H */
