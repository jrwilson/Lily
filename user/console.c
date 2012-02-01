#include "console.h"
#include "vga.h"
#include <automaton.h>
#include <string.h>

#define LIMIT 2
static int count = 0;
static unsigned short offset = 0;

void
init (int param,
      bd_t bd,
      size_t buffer_size,
      void* ptr)
{
  /* Schedule produce. */
  finish (CONSOLE_OP, 0, bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
op (const void* param,
    size_t bc)
{
  if ((count < LIMIT) && bc != 0) {
    ++count;
    bd_t bd = buffer_create (sizeof (op_t));
    op_t* op = buffer_map (bd);
    op->type = ASSIGN_VALUE;
    op->arg.assign_value.offset = offset;
    offset += 12;
    op->arg.assign_value.count = 12;
    op->arg.assign_value.data[0] = 0x3400 | 'H';
    op->arg.assign_value.data[1] = 0x3400 | 'e';
    op->arg.assign_value.data[2] = 0x3400 | 'l';
    op->arg.assign_value.data[3] = 0x3400 | 'l';
    op->arg.assign_value.data[4] = 0x3400 | 'o';
    op->arg.assign_value.data[5] = 0x3400 | ' ';
    op->arg.assign_value.data[6] = 0x3400 | 'w';
    op->arg.assign_value.data[7] = 0x3400 | 'o';
    op->arg.assign_value.data[8] = 0x3400 | 'r';
    op->arg.assign_value.data[9] = 0x3400 | 'l';
    op->arg.assign_value.data[10] = 0x3400 | 'd';
    op->arg.assign_value.data[11] = 0x3400 | '!';

    finish (CONSOLE_OP, 0, bd, sizeof (op_t), FINISH_DESTROY);
  }
  else {
    finish (NO_ACTION, 0, -1, 0, 0);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, CONSOLE_OP, op);
