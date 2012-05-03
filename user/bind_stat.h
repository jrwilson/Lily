#ifndef BIND_STAT_H
#define BIND_STAT_H

#include <lily/types.h>
#include <stdbool.h>

typedef struct bind_result bind_result_t;
typedef struct {
  bind_result_t* input_head;
  bind_result_t* output_head;
  bind_result_t* owner_head;
} bind_stat_t;

void
bind_stat_init (bind_stat_t* bs);

void
bind_stat_result (bind_stat_t* bs,
		  bd_t bda,
		  bd_t bdb);

bool
bind_stat_input_bound (bind_stat_t* bs,
		       ano_t ano,
		       int parameter);

bool
bind_stat_output_bound (bind_stat_t* bs,
			ano_t ano,
			int parameter);

#endif /* BIND_STAT_H */
