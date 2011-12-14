/*
  File
  ----
  trap_handler.hpp
  
  Description
  -----------
  Object for handling traps.
  
  Authors:
  Justin R. Wilson
*/

#include "trap_handler.hpp"
#include "idt.hpp"
#include "gdt.hpp"
#include "syscall_def.hpp"
#include "vm_def.hpp"
#include "system_automaton.hpp"
#include <utility>
#include "registers.hpp"

using namespace std::rel_ops;

/* Operating system trap starts at 128. */
static const unsigned int TRAP_BASE = 128;

extern "C" void trap0 ();

void
trap_handler::install ()
{
  idt::set (TRAP_BASE + 0, make_trap_gate (trap0, gdt::KERNEL_CODE_SELECTOR, descriptor_constants::RING0, descriptor_constants::PRESENT));
}

extern "C" void
trap_dispatch (volatile registers regs)
{
  switch (regs.eax) {
  case system::FINISH:
    {
      const void* action_entry_point = reinterpret_cast<const void*> (regs.ebx);
      aid_t parameter = static_cast<aid_t> (regs.ecx);
      bool output_status = regs.edx;
      const void* buffer = reinterpret_cast<const void*> (regs.esi);
      system_automaton::finish (action_entry_point, parameter, output_status, buffer);
      return;
    }
    break;
  case system::GETPAGESIZE:
    {
      regs.ebx = PAGE_SIZE;
      return;
    }
    break;
  case system::SBRK:
    {
      ptrdiff_t size = regs.ebx;
      regs.ebx = reinterpret_cast<uint32_t> (system_automaton::sbrk (size));
      return;
    }
    break;
  }
  
  system_automaton::unknown_system_call ();
}
