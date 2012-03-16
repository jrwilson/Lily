#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <lily/syscall.h>
#include <lily/types.h>
#include <lily/action.h>
#include <stddef.h>
#include <stdbool.h>

/* Import the Lily namespace. */
#define NO_PARAMETER LILY_ACTION_NO_PARAMETER
#define PARAMETER LILY_ACTION_PARAMETER
#define AUTO_PARAMETER LILY_ACTION_AUTO_PARAMETER

#define NO_ACTION LILY_ACTION_NO_ACTION

/* A macro for embedding the action information. */
#define EMBED_ACTION_DESCRIPTOR(action_type, parameter_mode, func_name, action_number, action_name, action_description) \
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
  ".long " #func_name "\n"		/* Action entry point. */		\
  ".long " quote (action_number) "\n"	/* Numeric identifier. */ \
  ".long 5f - 4f\n"			/* Size of the action name. */ \
  ".long 3f - 5f\n"			/* Size of the action description. */ \
  "4: .asciz \"" action_name "\"\n"		/* The action name. */ \
  "5: .asciz \"" action_description "\"\n"     /* The action description. */ \
  "3: .balign 4\n" \
       ".popsection\n");

#define BEGIN_SYSTEM_INPUT(action_no, action_name, action_desc, func, ano, param, bda, bdb) \
void func (ano, param, bda, bdb);					\
EMBED_ACTION_DESCRIPTOR (LILY_ACTION_SYSTEM_INPUT, PARAMETER, func, action_no, action_name, action_desc); \
void func (ano, param, bda, bdb)

#define BEGIN_INPUT(parameter_mode, action_no, action_name, action_desc, func, ano, param, bda, bdb) \
void func (ano, param, bda, bdb);					\
EMBED_ACTION_DESCRIPTOR (LILY_ACTION_INPUT, parameter_mode, func, action_no, action_name, action_desc); \
void func (ano, param, bda, bdb)

#define BEGIN_OUTPUT(parameter_mode, action_no, action_name, action_desc, func, ano, param) \
void func (ano, param);						\
EMBED_ACTION_DESCRIPTOR (LILY_ACTION_OUTPUT, parameter_mode, func, action_no, action_name, action_desc); \
void func (ano, param)

#define BEGIN_INTERNAL(parameter_mode, action_no, action_name, action_desc, func, ano, param) \
void func (ano, param);						\
EMBED_ACTION_DESCRIPTOR (LILY_ACTION_INTERNAL, parameter_mode, func, action_no, action_name, action_desc); \
void func (ano, param)

extern int automatonerrno;

void
finish (ano_t action_number,
	int parameter,
	bool output_fired,
	bd_t bda,
	bd_t bdb);

void
exit (void);

aid_t
create (bd_t text_bd,
	size_t text_size,
	bd_t bda,
	bd_t bdb,
	const char* name,
	size_t name_size,
	bool retain_privilege);

bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      int output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      int input_parameter);

int
unbind (bid_t bid);

int
destroy (aid_t aid);

int
subscribe_unbound (bid_t bid,
		   ano_t action_number);

int
unsubscribe_unbound (bid_t bid);


int
subscribe_destroyed (aid_t aid,
		     ano_t action_number);

int
unsubscribe_destroyed (aid_t aid);

void*
adjust_break (ptrdiff_t size);

bd_t
buffer_create (size_t size);

bd_t
buffer_copy (bd_t bd);

int
buffer_destroy (bd_t bd);

size_t
buffer_size (bd_t bd);

size_t
buffer_resize (bd_t bd,
	       size_t size);

int
buffer_assign (bd_t dest,
	       bd_t src);

size_t
buffer_append (bd_t dest,
	       bd_t src);

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

size_t
size_to_pages (size_t size);

int
syslog (const char* msg);

int
syslogn (const char* msg,
	 size_t size);

aid_t
lookup (const char* name,
	size_t size);

bd_t
describe (aid_t aid);

aid_t
getaid (void);

bd_t
getinita (void);

bd_t
getinitb (void);

int
getmonotime (mono_time_t* t);

#endif /* AUTOMATON_H */
