#include "kput.h"
#include "kassert.h"
#include "syscall.h"

asm (".pushsection .action\n"
     "alice: .long 0x1234\n"
     "bob: .long 0x5678\n"
     ".popsection\n");

asm (".global bang_entry\n"
     "bang_entry:\n"
     "push %eax\n"
     "call driver");

extern unsigned int bang_entry;
static unsigned int counter = 0;

static void
driver (unsigned int parameter)
{
  /* Precondition. */
  if (counter < 10) {
    /* Effect. */
    kputs ("counter = "); kputuix (counter); kputs ("\n");
    ++counter;
  }

  /* Precondition. */
  if (counter < 10) {
    /* Schedule. */
    syscall_t syscall = SYSCALL_SCHEDULE;
    unsigned int action_entry_point = (unsigned int)&bang_entry;
    unsigned int parameter = 0;
    asm volatile ("mov %0, %%eax\n"
		  "mov %1, %%ebx\n"
		  "mov %2, %%ecx\n"
		  "int $0x80\n" : : "m"(syscall), "m"(action_entry_point), "m"(parameter));
  }
  else {
    /* Don't schedule. */
    /* Schedule. */
    syscall_t syscall = SYSCALL_FINISH;
    asm volatile ("mov %0, %%eax\n"
		  "int $0x80\n" : : "m"(syscall));
  }
}
