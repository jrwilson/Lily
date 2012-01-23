#ifndef LILY_ACTION_H
#define LILY_ACTION_H

#include <lily/quote.h>

#define LILY_ACTION_NO_COMPARE 0
#define LILY_ACTION_EQUAL 1

#define LILY_ACTION_INPUT 0
#define LILY_ACTION_OUTPUT 1
#define LILY_ACTION_INTERNAL 2

#define LILY_ACTION_NO_PARAMETER 0
#define LILY_ACTION_PARAMETER 1
#define LILY_ACTION_AUTO_PARAMETER 2

/* A macro for embedding the action information. */
#define EMBED_ACTION_DESCRIPTOR(base, func_name)				\
  __asm__ (".pushsection .action_info, \"\", @note\n"		   \
  ".balign 4\n"							   \
  ".long 1f - 0f\n"		/* Length of the author string. */ \
  ".long 3f - 2f\n"		/* Length of the description. */ \
  ".long 0\n"			/* The type.  0 => action descriptor. */ \
  "0: .asciz \"lily\"\n"	/* The author of the note. */ \
  "1: .balign 4\n" \
  "2:\n"			/* The description of the note. */ \
  ".long 5f - 4f\n"		/* Length of the action name. */ \
  ".long 3f - 5f\n"		/* Length of the action description. */ \
  ".long " quote (base##_COMPARE) "\n"			/* Action description comparison method. 0 => string compare. */ \
  ".long " quote (base##_ACTION) "\n"	/* Action type. */ \
  ".long " #func_name "\n"		/* Action entry point. */		\
  ".long " quote (base##_PARAMETER) "\n"	/* Parameter mode. */ \
  "4: .asciz \"" base##_NAME "\"\n"		/* The name of this action. */ \
  "5: .asciz \"" base##_DESCRIPTION "\"\n"	/* The description of this action. */\
  "3: .balign 4\n" \
       ".popsection\n");

#endif /* LILY_ACTION_H */


