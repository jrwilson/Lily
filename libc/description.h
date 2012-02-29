#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include <lily/types.h>
#include <buffer_file.h>

typedef struct {
  aid_t aid;
  buffer_file_t bf;
} description_t;

int
description_init (description_t* d,
		  aid_t aid);

void
description_fini (description_t* d);

ano_t
description_name_to_number (description_t* d,
			    const char* action_name,
			    size_t size);

#endif /* DESCRIPTION_H */
