#include <automaton.h>

#define syscall0(syscall)		\
  __asm__ ("mov %0, %%eax\n" \
	   "int $0x80\n" : : "g"(syscall) : "eax", "ecx");

#define syscall3(syscall, p1, p2, p3)		\
  __asm__ ("mov %0, %%eax\n" \
	   "mov %1, %%ebx\n" \
	   "mov %2, %%ecx\n" \
	   "mov %3, %%edx\n" \
	   "int $0x80\n" : : "g"(syscall), "m"(p1), "m"(p2), "m"(p3) : "eax", "ebx", "ecx");

#define syscall0r(syscall, retval)	\
  __asm__ ("mov %1, %%eax\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" : "=g"(retval) : "g"(syscall) : "eax", "ecx" );

#define syscall1re(syscall, retval, p1, err)	\
  lily_error_t e; \
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(e) : "g"(syscall), "g"(p1) : "eax", "ebx", "ecx" ); \
  if (err != 0) { \
    *err = e; \
  }

#define syscall2re(syscall, retval, p1, p2, err)	\
  lily_error_t e; \
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "mov %4, %%ecx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(e) : "g"(syscall), "m"(p1), "m"(p2) : "eax", "ebx", "ecx" ); \
  if (err != 0) { \
    *err = e; \
  }

#define syscall3re(syscall, retval, p1, p2, p3, err)	\
  lily_error_t e; \
  __asm__ ("mov %2, %%eax\n" \
	   "mov %3, %%ebx\n" \
	   "mov %4, %%ecx\n" \
	   "mov %5, %%edx\n" \
	   "int $0x80\n" \
	   "mov %%eax, %0\n" \
	   "mov %%ecx, %1\n" : "=g"(retval), "=g"(e) : "g"(syscall), "m"(p1), "m"(p2), "m"(p3) : "eax", "ebx", "ecx", "edx" ); \
  if (err != 0) { \
    *err = e; \
  }

int
schedule (ano_t action_number,
	  int parameter,
	  lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_SCHEDULE, retval, action_number, parameter, err);
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
    buffer_destroy (bda, 0);
  }
  if (bdb != -1) {
    buffer_destroy (bdb, 0);
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
exit (void)
{
  syscall0 (LILY_SYSCALL_EXIT);
}

aid_t
create (bd_t text_bd,
	size_t text_size,
	bd_t bda,
	bd_t bdb,
	const char* name,
	size_t name_size,
	bool retain_privilege,
	lily_error_t* err)
{
  aid_t retval;
  syscall1re (LILY_SYSCALL_CREATE, retval, &text_bd, err);
  return retval;
}

bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      int output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      int input_parameter,
      lily_error_t* err)
{
  aid_t retval;
  syscall1re (LILY_SYSCALL_BIND, retval, &output_automaton, err);
  return retval;
}

int
unbind (bid_t bid,
	lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNBIND, retval, bid, err);
  return retval;
}

int
destroy (aid_t aid,
	 lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_DESTROY, retval, aid, err);
  return retval;
}

int
subscribe_unbound (bid_t bid,
		   ano_t action_number,
		   lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_SUBSCRIBE_UNBOUND, retval, bid, action_number, err);
  return retval;
}

int
unsubscribe_unbound (bid_t bid,
		     lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNSUBSCRIBE_UNBOUND, retval, bid, err);
  return retval;
}

int
subscribe_destroyed (aid_t aid,
		     ano_t action_number,
		     lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_SUBSCRIBE_DESTROYED, retval, aid, action_number, err);
  return retval;
}

int
unsubscribe_destroyed (aid_t aid,
		       lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNSUBSCRIBE_DESTROYED, retval, aid, err);
  return retval;
}

void*
adjust_break (ptrdiff_t size,
	      lily_error_t* err)
{
  void* retval;
  syscall1re (LILY_SYSCALL_ADJUST_BREAK, retval, size, err);
  return retval;
}

bd_t
buffer_create (size_t size,
	       lily_error_t* err)
{
  bd_t retval;
  syscall1re (LILY_SYSCALL_BUFFER_CREATE, retval, size, err);
  return retval;
}

bd_t
buffer_copy (bd_t bd,
	     lily_error_t* err)
{
  bd_t retval;
  syscall1re (LILY_SYSCALL_BUFFER_COPY, retval, bd, err);
  return retval;
}

int
buffer_destroy (bd_t bd,
		lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_BUFFER_DESTROY, retval, bd, err);
  return retval;
}

size_t
buffer_size (bd_t bd,
	     lily_error_t* err)
{
  size_t retval;
  syscall1re (LILY_SYSCALL_BUFFER_SIZE, retval, bd, err);
  return retval;
}

size_t
buffer_resize (bd_t bd,
	       size_t size,
	       lily_error_t* err)
{
  size_t retval;
  syscall2re (LILY_SYSCALL_BUFFER_RESIZE, retval, bd, size, err);
  return retval;
}

int
buffer_assign (bd_t dest,
	       bd_t src,
	       lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_BUFFER_ASSIGN, retval, dest, src, err);
  return retval;
}

size_t
buffer_append (bd_t dest,
	       bd_t src,
	       lily_error_t* err)
{
  size_t retval;
  syscall2re (LILY_SYSCALL_BUFFER_APPEND, retval, dest, src, err);
  return retval;
}

void*
buffer_map (bd_t bd,
	    lily_error_t* err)
{
  void* retval;
  syscall1re (LILY_SYSCALL_BUFFER_MAP, retval, bd, err);
  return retval;
}

int
buffer_unmap (bd_t bd,
	      lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_BUFFER_UNMAP, retval, bd, err);
  return retval;
}

long
sysconf (int name,
	 lily_error_t* err)
{
  long retval;
  syscall1re (LILY_SYSCALL_SYSCONF, retval, name, err);
  return retval;
}

static size_t page_size = 0;

size_t
pagesize (void)
{
  if (page_size == 0) {
    page_size = sysconf (SYSCONF_PAGESIZE, 0);
  }
  return page_size;
}

size_t
size_to_pages (size_t size)
{
  size_t ps = pagesize ();
  return ALIGN_UP (size, ps) / ps;
}

aid_t
lookup (const char* name,
	size_t size,
	lily_error_t* err)
{
  aid_t retval;
  syscall2re (LILY_SYSCALL_LOOKUP, retval, name, size, err);
  return retval;
}

bd_t
describe (aid_t aid,
	  lily_error_t* err)
{
  bd_t retval;
  syscall1re (LILY_SYSCALL_DESCRIBE, retval, aid, err);
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
getmonotime (mono_time_t* t,
	     lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_GETMONOTIME, retval, t, err);
  return retval;
}

int
map (const void* destination,
     const void* source,
     size_t size,
     lily_error_t* err)
{
  int retval;
  syscall3re (LILY_SYSCALL_MAP, retval, destination, source, size, err);
  return retval;
}

int
unmap (const void* destination,
       lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNMAP, retval, destination, err);
  return retval;
}

int
reserve_port (unsigned short port,
	      lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_RESERVE_PORT, retval, port, err);
  return retval;
}

int
unreserve_port (unsigned short port,
		lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNRESERVE_PORT, retval, port, err);
  return retval;
}

unsigned char
inb (unsigned short port,
     lily_error_t* err)
{
  unsigned int retval;
  syscall1re (LILY_SYSCALL_INB, retval, port, err);
  return retval;
}

int
outb (unsigned short port,
      unsigned char value,
      lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_OUTB, retval, port, value, err);
  return retval;
}

unsigned short
inw (unsigned short port,
     lily_error_t* err)
{
  unsigned int retval;
  syscall1re (LILY_SYSCALL_INW, retval, port, err);
  return retval;
}

int
outw (unsigned short port,
      unsigned short value,
      lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_OUTW, retval, port, value, err);
  return retval;
}

unsigned long
inl (unsigned short port,
     lily_error_t* err)
{
  unsigned long retval;
  syscall1re (LILY_SYSCALL_INL, retval, port, err);
  return retval;
}

int
outl (unsigned short port,
      unsigned long value,
      lily_error_t* err)
{
  int retval;
  syscall2re (LILY_SYSCALL_OUTL, retval, port, value, err);
  return retval;
}

int
subscribe_irq (int irq,
	       ano_t ano,
	       int param,
	       lily_error_t* err)
{
  int retval;
  syscall3re (LILY_SYSCALL_SUBSCRIBE_IRQ, retval, irq, ano, param, err);
  return retval;
}

int
unsubscribe_irq (int irq,
		 lily_error_t* err)
{
  int retval;
  syscall1re (LILY_SYSCALL_UNSUBSCRIBE_IRQ, retval, irq, err);
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
