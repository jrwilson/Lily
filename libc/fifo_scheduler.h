#ifndef FIFO_SCHEDULER_H
#define FIFO_SCHEDULER_H

#include <lily/types.h>
#include <stddef.h>
#include <stdbool.h>

void
scheduler_add (ano_t action_number,
	       int parameter);

void
scheduler_remove (ano_t action_number,
		  int parameter);
void
scheduler_finish (bool output_fired,
		  bd_t bd);

#endif /* FIFO_SCHEDULER_H */
