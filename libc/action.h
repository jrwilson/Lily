#ifndef ACTION_H
#define ACTION_H

#include <lily/action.h>

/* Import the Lily namespace. */

#define INPUT LILY_ACTION_INPUT
#define OUTPUT LILY_ACTION_OUTPUT
#define INTERNAL LILY_ACTION_INTERNAL

#define NO_PARAMETER LILY_ACTION_NO_PARAMETER
#define PARAMETER LILY_ACTION_PARAMETER
#define AUTO_PARAMETER LILY_ACTION_AUTO_PARAMETER

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

#endif /* ACTION_H */
