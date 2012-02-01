#include "producer.h"
#include <automaton.h>
#include <string.h>
#include "console.h"

/* #define LIMIT 3 */
/* static int count = 0; */

/* const char* string = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n"; */

/* void */
/* init (void) */
/* { */
/*   /\* Schedule produce. *\/ */
/*   finish (PRODUCER_PRINT, 0, -1, 0, 0); */
/* } */
/* EMBED_ACTION_DESCRIPTOR (INTERNAL, NO_PARAMETER, LILY_ACTION_INIT, init); */

/* void */
/* print (void) */
/* { */
/*   if (count < LIMIT) { */
/*     ++count; */
/*     size_t len = strlen (string); */
/*     bd_t bd = buffer_create (len); */
/*     char* buffer = buffer_map (bd); */
/*     memcpy (buffer, string, len); */
/*     finish (PRODUCER_PRINT, 0, bd, len, FINISH_DESTROY); */
/*   } */
/*   else { */
/*     finish (NO_ACTION, 0, -1, 0, 0); */
/*   } */
/* } */
/* EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_PRINT, print); */

/* void */
/* print_sense (void) */
/* { */
/*   finish (NO_ACTION, 0, -1, 0, 0); */
/* } */
/* EMBED_ACTION_DESCRIPTOR (INPUT, NO_PARAMETER, PRODUCER_PRINT_SENSE, print_sense); */

#define LIMIT 4
static int count = 0;
static unsigned short offset = 0;

void
init (int param,
      bd_t bd,
      size_t buffer_size,
      void* ptr)
{
  finish (NO_ACTION, 0, bd, buffer_size, FINISH_DESTROY);
}
EMBED_ACTION_DESCRIPTOR (SYSTEM_INPUT, NO_PARAMETER, INIT, init);

void
producer_console_op (const void* param,
		size_t bc)
{
  if ((count < LIMIT) && bc != 0) {
    ++count;

    bd_t bd;
    size_t size;
    if (count % 2 != 0) {
      size = sizeof (console_op_t) + sizeof (unsigned short) * 12;
      bd = buffer_create (size);
      console_op_t* op = buffer_map (bd);
      op->type = CONSOLE_ASSIGN;
      op->arg.assign.offset = offset;
      offset += sizeof (unsigned short) * 12;
      op->arg.assign.size = sizeof (unsigned short) * 12;
      unsigned short* data = (unsigned short*)&op->arg.assign.data[0];
      data[0] = 0x3400 | 'J';
      data[1] = 0x3400 | 'e';
      data[2] = 0x3400 | 'l';
      data[3] = 0x3400 | 'l';
      data[4] = 0x3400 | 'o';
      data[5] = 0x3400 | ' ';
      data[6] = 0x3400 | 'w';
      data[7] = 0x3400 | 'o';
      data[8] = 0x3400 | 'r';
      data[9] = 0x3400 | 'l';
      data[10] = 0x3400 | 'd';
      data[11] = 0x3400 | '!';
    }
    else {
      size = sizeof (console_op_t);
      bd = buffer_create (size);
      console_op_t* op = buffer_map (bd);
      op->type = CONSOLE_COPY;
      op->arg.copy.dst_offset = offset;
      offset += sizeof (unsigned short) * 12;
      op->arg.copy.src_offset = 0;
      op->arg.copy.size = sizeof (unsigned short) * 12;
    }
    finish (PRODUCER_CONSOLE_OP, 0, bd, size, FINISH_DESTROY);
  }
  else {
    finish (NO_ACTION, 0, -1, 0, FINISH_NO);
  }
}
EMBED_ACTION_DESCRIPTOR (OUTPUT, NO_PARAMETER, PRODUCER_CONSOLE_OP, producer_console_op);
