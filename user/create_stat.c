#include "create_stat.h"

#include <automaton.h>
#include <buffer_file.h>
#include "system_msg.h"
#include <dymem.h>
#include <string.h>

struct create_result {
  sa_create_result_t result;
  create_result_t* next;
};

void
create_stat_init (create_stat_t* bs)
{
  bs->owner_head = 0;
}

void
create_stat_result (create_stat_t* bs,
		    bd_t bda,
		    bd_t bdb)
{
  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }
  
  const sa_create_result_t* res = buffer_file_readp (&bf, sizeof (sa_create_result_t));
  if (res == 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }
  
  if (res->outcome == SA_CREATE_SUCCESS) {
    create_result_t* br = malloc (sizeof (create_result_t));
    memset (br, 0, sizeof (create_result_t));
    br->result = *res;
    
    br->next = bs->owner_head;
    bs->owner_head = br;
  }

  finish_input (bda, bdb);
}
