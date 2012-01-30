#include "console.h"
#include "vga.h"
#include <automaton.h>
#include <string.h>

#define LIMIT 2
static int count = 0;
static unsigned short offset = 0;

void
init (void)
{
  /* Schedule produce. */
  finish (CONSOLE_OP, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init);

void
op (void)
{
  if (count < LIMIT && binding_count (CONSOLE_OP, 0) != 0 && binding_count (CONSOLE_OP_SENSE, 0) != 0) {
    ++count;
    op_t op;

    op.type = ASSIGN_VALUE;
    op.arg.assign_value.offset = offset;
    offset += 12;
    op.arg.assign_value.count = 12;
    op.arg.assign_value.data[0] = 0x3400 | 'H';
    op.arg.assign_value.data[1] = 0x3400 | 'e';
    op.arg.assign_value.data[2] = 0x3400 | 'l';
    op.arg.assign_value.data[3] = 0x3400 | 'l';
    op.arg.assign_value.data[4] = 0x3400 | 'o';
    op.arg.assign_value.data[5] = 0x3400 | ' ';
    op.arg.assign_value.data[6] = 0x3400 | 'w';
    op.arg.assign_value.data[7] = 0x3400 | 'o';
    op.arg.assign_value.data[8] = 0x3400 | 'r';
    op.arg.assign_value.data[9] = 0x3400 | 'l';
    op.arg.assign_value.data[10] = 0x3400 | 'd';
    op.arg.assign_value.data[11] = 0x3400 | '!';

    finish (CONSOLE_OP, 0, &op, sizeof (op_t), -1, 0);
  }
  else {
    finish (0, 0, 0, 0, -1, 0);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, CONSOLE_OP, op);

void
op_sense (void)
{
  finish (CONSOLE_OP, 0, 0, 0, -1, 0);
}
EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, CONSOLE_OP_SENSE, op_sense);
