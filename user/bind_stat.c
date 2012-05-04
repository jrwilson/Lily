#include "bind_stat.h"

#include <automaton.h>
#include <buffer_file.h>
#include "system_msg.h"
#include <dymem.h>
#include <string.h>

struct bind_result {
  sa_bind_result_t result;
  bind_result_t* next;
};

void
bind_stat_init (bind_stat_t* bs)
{
  bs->input_head = 0;
  bs->output_head = 0;
  bs->owner_head = 0;
}

void
bind_stat_result (bind_stat_t* bs,
		  bd_t bda,
		  bd_t bdb)
{
  buffer_file_t bf;
  if (buffer_file_initr (&bf, bda) != 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }
  
  const sa_bind_result_t* res = buffer_file_readp (&bf, sizeof (sa_bind_result_t));
  if (res == 0) {
    /* TODO:  log? */
    finish_input (bda, bdb);
  }

  switch (res->role) {
  case SA_BIND_INPUT:
  case SA_BIND_OUTPUT:
  case SA_BIND_OWNER:
    break;
  default:
    /* TODO:  log? */
    finish_input (bda, bdb);
    break;
  }

  if (res->outcome == SA_BIND_SUCCESS) {
    bind_result_t* br = malloc (sizeof (bind_result_t));
    memset (br, 0, sizeof (bind_result_t));
    br->result = *res;
    
    switch (res->role) {
    case SA_BIND_INPUT:
      br->next = bs->input_head;
      bs->input_head = br;
      break;
    case SA_BIND_OUTPUT:
      br->next = bs->output_head;
      bs->output_head = br;
      break;
    case SA_BIND_OWNER:
      br->next = bs->owner_head;
      bs->owner_head = br;
      break;
    }
  }

  finish_input (bda, bdb);
}

bool
bind_stat_input_bound (bind_stat_t* bs,
		       ano_t ano,
		       int parameter)
{
  for (bind_result_t* ptr = bs->input_head; ptr != 0; ptr = ptr->next) {
    if (ptr->result.binding.input_ano == ano && ptr->result.binding.input_parameter == parameter) {
      return true;
    }
  }
  return false;
}

bool
bind_stat_output_bound (bind_stat_t* bs,
			ano_t ano,
			int parameter)
{

  for (bind_result_t* ptr = bs->output_head; ptr != 0; ptr = ptr->next) {
    if (ptr->result.binding.output_ano == ano && ptr->result.binding.output_parameter == parameter) {
      return true;
    }
  }
  return false;
}
