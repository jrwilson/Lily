#include "automaton.h"
#include "string.h"

lily_error_t lily_error = LILY_ERROR_SUCCESS;

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

int
schedule (ano_t action_number,
	  int parameter)
{
  int retval;
  syscall2re (LILY_SYSCALL_SCHEDULE, retval, action_number, parameter);
  return retval;
}

void
finish (bool output_fired,
	bd_t bda,
	bd_t bdb)
{
  syscall3 (LILY_SYSCALL_FINISH, output_fired, bda, bdb);
}

void
do_schedule (void);

void
finish_input (bd_t bda,
	      bd_t bdb)
{
  if (bda != -1) {
    buffer_destroy (bda);
  }
  if (bdb != -1) {
    buffer_destroy (bdb);
  }
  do_schedule ();
  finish (false, -1, -1);
}

void
finish_output (bool output_fired,
	       bd_t bda,
	       bd_t bdb)
{
  do_schedule ();
  finish (output_fired, bda, bdb);
}

void
finish_internal (void)
{
  do_schedule ();
  finish (false, -1, -1);
}

void
exit (int code)
{
  syscall1 (LILY_SYSCALL_EXIT, code);
}

aid_t
create (bd_t text_bd,
	bd_t bda,
	bd_t bdb,
	bool retain_privilege)
{
  aid_t retval;
  syscall1re (LILY_SYSCALL_CREATE, retval, &text_bd);
  return retval;
}

bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      int output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      int input_parameter)
{
  aid_t retval;
  syscall1re (LILY_SYSCALL_BIND, retval, &output_automaton);
  return retval;
}

int
unbind (bid_t bid)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNBIND, retval, bid);
  return retval;
}

int
destroy (aid_t aid)
{
  int retval;
  syscall1re (LILY_SYSCALL_DESTROY, retval, aid);
  return retval;
}

int
log (const char* message,
     size_t message_size)
{
  int retval;
  syscall2re (LILY_SYSCALL_LOG, retval, message, message_size);
  return retval;
}

int
logs (const char* message)
{
  return log (message, strlen (message) + 1);
}

void*
adjust_break (ptrdiff_t size)
{
  void* retval;
  syscall1re (LILY_SYSCALL_ADJUST_BREAK, retval, size);
  return retval;
}

bd_t
buffer_create (size_t size)
{
  bd_t retval;
  syscall1re (LILY_SYSCALL_BUFFER_CREATE, retval, size);
  return retval;
}

bd_t
buffer_copy (bd_t bd)
{
  bd_t retval;
  syscall1re (LILY_SYSCALL_BUFFER_COPY, retval, bd);
  return retval;
}

int
buffer_destroy (bd_t bd)
{
  int retval;
  syscall1re (LILY_SYSCALL_BUFFER_DESTROY, retval, bd);
  return retval;
}

size_t
buffer_size (bd_t bd)
{
  size_t retval;
  syscall1re (LILY_SYSCALL_BUFFER_SIZE, retval, bd);
  return retval;
}

size_t
buffer_resize (bd_t bd,
	       size_t size)
{
  size_t retval;
  syscall2re (LILY_SYSCALL_BUFFER_RESIZE, retval, bd, size);
  return retval;
}

int
buffer_assign (bd_t dest,
	       bd_t src)
{
  int retval;
  syscall2re (LILY_SYSCALL_BUFFER_ASSIGN, retval, dest, src);
  return retval;
}

size_t
buffer_append (bd_t dest,
	       bd_t src)
{
  size_t retval;
  syscall2re (LILY_SYSCALL_BUFFER_APPEND, retval, dest, src);
  return retval;
}

void*
buffer_map (bd_t bd)
{
  void* retval;
  syscall1re (LILY_SYSCALL_BUFFER_MAP, retval, bd);
  return retval;
}

int
buffer_unmap (bd_t bd)
{
  int retval;
  syscall1re (LILY_SYSCALL_BUFFER_UNMAP, retval, bd);
  return retval;
}

long
sysconf (int name)
{
  long retval;
  syscall1re (LILY_SYSCALL_SYSCONF, retval, name);
  return retval;
}

static size_t page_size = 0;

size_t
pagesize (void)
{
  if (page_size == 0) {
    page_size = sysconf (SYSCONF_PAGESIZE);
  }
  return page_size;
}

size_t
size_to_pages (size_t size)
{
  size_t ps = pagesize ();
  return ALIGN_UP (size, ps) / ps;
}

bd_t
describe (aid_t aid)
{
  bd_t retval;
  syscall1re (LILY_SYSCALL_DESCRIBE, retval, aid);
  return retval;
}

aid_t
getaid (void)
{
  aid_t retval;
  syscall0r (LILY_SYSCALL_GETAID, retval);
  return retval;
}

bd_t
getinita (void)
{
  bd_t retval;
  syscall0r (LILY_SYSCALL_GETINITA, retval);
  return retval;
}

bd_t
getinitb (void)
{
  bd_t retval;
  syscall0r (LILY_SYSCALL_GETINITB, retval);
  return retval;
}

int
getmonotime (mono_time_t* t)
{
  int retval;
  syscall1re (LILY_SYSCALL_GETMONOTIME, retval, t);
  return retval;
}

int
map (const void* destination,
     const void* source,
     size_t size)
{
  int retval;
  syscall3re (LILY_SYSCALL_MAP, retval, destination, source, size);
  return retval;
}

int
unmap (const void* destination)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNMAP, retval, destination);
  return retval;
}

int
reserve_port (unsigned short port)
{
  int retval;
  syscall1re (LILY_SYSCALL_RESERVE_PORT, retval, port);
  return retval;
}

int
unreserve_port (unsigned short port)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNRESERVE_PORT, retval, port);
  return retval;
}

unsigned char
inb (unsigned short port)
{
  unsigned int retval;
  syscall1re (LILY_SYSCALL_INB, retval, port);
  return retval;
}

int
outb (unsigned short port,
      unsigned char value)
{
  int retval;
  syscall2re (LILY_SYSCALL_OUTB, retval, port, value);
  return retval;
}

unsigned short
inw (unsigned short port)
{
  unsigned int retval;
  syscall1re (LILY_SYSCALL_INW, retval, port);
  return retval;
}

int
outw (unsigned short port,
      unsigned short value)
{
  int retval;
  syscall2re (LILY_SYSCALL_OUTW, retval, port, value);
  return retval;
}

unsigned long
inl (unsigned short port)
{
  unsigned long retval;
  syscall1re (LILY_SYSCALL_INL, retval, port);
  return retval;
}

int
outl (unsigned short port,
      unsigned long value)
{
  int retval;
  syscall2re (LILY_SYSCALL_OUTL, retval, port, value);
  return retval;
}

int
subscribe_irq (int irq,
	       ano_t ano,
	       int param)
{
  int retval;
  syscall3re (LILY_SYSCALL_SUBSCRIBE_IRQ, retval, irq, ano, param);
  return retval;
}

int
unsubscribe_irq (int irq)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNSUBSCRIBE_IRQ, retval, irq);
  return retval;
}

const char*
lily_error_string (lily_error_t err)
{
  switch (err) {
  case LILY_ERROR_SUCCESS:
    return "success";
  case LILY_ERROR_INVAL:
    return "invalid input value";
  case LILY_ERROR_ALREADY:
    return "resource already in use";
  case LILY_ERROR_NOT:
    return "no effect";
  case LILY_ERROR_PERMISSION:
    return "operation not permitted";
  case LILY_ERROR_AIDDNE:
    return "automaton does not exist";
  case LILY_ERROR_BIDDNE:
    return "binding does not exist";
  case LILY_ERROR_ANODNE:
    return "action does not exist";
  case LILY_ERROR_BDDNE:
    return "buffer does not exist";
  case LILY_ERROR_NOMEM:
    return "out of memory";
  case LILY_ERROR_OAIDDNE:
    return "output automaton does not exist";
  case LILY_ERROR_IAIDDNE:
    return "input automaton does not exist";
  case LILY_ERROR_OANODNE:
    return "output action does not exist";
  case LILY_ERROR_IANODNE:
    return "input action does not exist";
  }
  
  return "unknown error";
}
