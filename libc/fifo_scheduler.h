#ifndef FIFO_SCHEDULER_H
#define FIFO_SCHEDULER_H

#include <lily/types.h>
#include <stddef.h>

void
scheduler_add (ano_t action_number,
	       int parameter);

void
scheduler_remove (ano_t action_number,
		  int parameter);
void
scheduler_finish (bd_t bd,
		  size_t buffer_size,
		  int flags);

#endif /* FIFO_SCHEDULER_H */
