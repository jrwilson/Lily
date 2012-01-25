#ifndef FINISH_H
#define FINISH_H

#include <lily/types.h>
#include <stddef.h>

void
finish (const void* action_entry_point,
	const void* parameter,
	const void* value,
	size_t value_size,
	bd_t buffer,
	size_t buffer_size);

#endif /* FINISH_H */
