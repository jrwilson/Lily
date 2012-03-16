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
end_input_action (bd_t bda,
		  bd_t bdb);

void
end_output_action (bool output_fired,
		   bd_t bda,
		   bd_t bdb);

void
end_internal_action (void);

#endif /* FIFO_SCHEDULER_H */
