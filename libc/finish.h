#ifndef FINISH_H
#define FINISH_H

#include <lily/types.h>
#include <stddef.h>

extern void
finish (const void* action_entry_point,
	const void* parameter,
	const void* value,
	size_t value_size,
	bid_t buffer,
	size_t buffer_size);

#endif /* FINISH_H */
