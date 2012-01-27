#ifndef FINISH_H
#define FINISH_H

#include <lily/types.h>
#include <stddef.h>

void
finish (ano_t action_number,
	const void* parameter,
	const void* value,
	size_t value_size,
	bd_t buffer,
	size_t buffer_size);

#endif /* FINISH_H */
