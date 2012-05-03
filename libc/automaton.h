#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <lily/syscall.h>
#include <lily/types.h>
#include <lily/action.h>
#include <stddef.h>

/* Import the Lily namespace. */
#define NO_PARAMETER LILY_ACTION_NO_PARAMETER
#define PARAMETER LILY_ACTION_PARAMETER
#define AUTO_PARAMETER LILY_ACTION_AUTO_PARAMETER

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

#define syscall0(syscall)		\
  __asm__ ("mov %0, %%eax\n" \
	   "int $0x80\n" : : "g"(syscall) : "eax", "ecx");

#define syscall1(syscall, p1)		\
  __asm__ ("mov %0, %%eax\n" \
	   "mov %1, %%ebx\n" \
	   "int $0x80\n" : : "g"(syscall), "m"(p1) : "eax", "ebx", "ecx");

#define syscall2(syscall, p1, p2)		\
  __asm__ ("mov %0, %%eax\n" \
	   "mov %1, %%ebx\n" \
	   "mov %2, %%ecx\n" \
	   "int $0x80\n" : : "g"(syscall), "m"(p1), "m"(p2) : "eax", "ebx", "ecx");

#define syscall3(syscall, p1, p2, p3)		\
  __asm__ ("mov %0, %%eax\n" \
	   "mov %1, %%ebx\n" \
	   "mov %2, %%ecx\n" \
	   "mov %3, %%edx\n" \
	   "int $0x80\n" : : "g"(syscall), "m"(p1), "m"(p2), "m"(p3) : "eax", "ebx", "ecx", "edx");

#define syscall0r(syscall, retval)	\
  __asm__ ("mov %1, %%eax\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" : "=g"(retval) : "g"(syscall) : "eax", "ecx" );

#define syscall1r(syscall, p1, retval)		\
  __asm__ ("mov %1, %%eax\n" \
           "mov %2, %%ebx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" : "=g"(retval) : "g"(syscall), "m"(p1) : "eax", "ebx", "ecx" );

#define syscall2r(syscall, p1, p2, retval)	\
  __asm__ ("mov %1, %%eax\n" \
           "mov %2, %%ebx\n" \
           "mov %3, %%ecx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" : "=g"(retval) : "g"(syscall), "m"(p1), "m"(p2) : "eax", "ebx", "ecx" );

#define syscall0re(syscall, retval)	\
  __asm__ ("mov %2, %%eax\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(lily_error) : "g"(syscall) : "eax", "ecx" );

#define syscall1re(syscall, retval, p1)	\
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(lily_error) : "g"(syscall), "g"(p1) : "eax", "ebx", "ecx" );

#define syscall2re(syscall, retval, p1, p2)	\
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "mov %4, %%ecx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(lily_error) : "g"(syscall), "m"(p1), "m"(p2) : "eax", "ebx", "ecx" );

#define syscall3re(syscall, retval, p1, p2, p3)	\
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "mov %4, %%ecx\n" \
	   "mov %5, %%edx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(lily_error) : "g"(syscall), "m"(p1), "m"(p2), "m"(p3) : "eax", "ebx", "ecx", "edx" );

#define syscall4re(syscall, retval, p1, p2, p3, p4)	\
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "mov %4, %%ecx\n" \
	   "mov %5, %%edx\n" \
	   "mov %6, %%esi\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(lily_error) : "g"(syscall), "m"(p1), "m"(p2), "m"(p3), "m"(p4) : "eax", "ebx", "ecx", "edx", "esi" );

extern lily_error_t lily_error;

int
schedule (ano_t action_number,
	  int parameter);

void
finish (int output_fired,
	bd_t bda,
	bd_t bdb);

void
finish_input (bd_t bda,
	      bd_t bdb);

void
finish_output (int output_fired,
	       bd_t bda,
	       bd_t bdb);

void
finish_internal (void);

void
exit (int code);

int
log (const char* message,
     size_t message_size);

int
logs (const char* message);

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
	       bd_t src,
	       size_t begin,
	       size_t end);

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

/* These calls can only be made by privileged automata. */
int
map (const void* destination,
     const void* source,
     size_t size);

int
unmap (const void* destination);

int
reserve_port (unsigned short port);

int
unreserve_port (unsigned short port);

unsigned char
inb (unsigned short port);

int
outb (unsigned short port,
      unsigned char value);

unsigned short
inw (unsigned short port);

int
outw (unsigned short port,
      unsigned short value);

unsigned long
inl (unsigned short port);

int
outl (unsigned short port,
      unsigned long value);

int
subscribe_irq (int irq,
	       ano_t ano,
	       int param);

int
unsubscribe_irq (int irq);

const char*
lily_error_string (lily_error_t err);

#endif /* AUTOMATON_H */
