#include <automaton.h>
#include <io.h>

#define KEYBOARD_INTERRUPT 1
#define KEYBOARD_KEYCODE 2

#define KEYBOARD_PORT 0x64
#define KEYBOARD_IRQ 1

void
init (int param,
      bd_t bd,
      void* ptr,
      size_t buffer_size)
{
  reserve_port (KEYBOARD_PORT);
  subscribe_irq (KEYBOARD_IRQ, KEYBOARD_INTERRUPT, 0);
  finish (NO_ACTION, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
interrupt (int param)
{
  unsigned char c = inb (KEYBOARD_PORT);
  finish (NO_ACTION, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, KEYBOARD_INTERRUPT, interrupt);

void
keycode (int param,
	 size_t binding_count)
{
  finish (NO_ACTION, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, KEYBOARD_KEYCODE, keycode);
