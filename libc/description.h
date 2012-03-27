#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include <lily/types.h>
#include <buffer_file.h>

typedef struct {
  aid_t aid;
  buffer_file_t bf;
} description_t;

typedef struct {
  int type;
  int parameter_mode;
  ano_t number;
  const char* name;
  size_t name_size;
  const char* description;
  size_t description_size;
} action_desc_t;

int
description_init (description_t* d,
		  aid_t aid);

void
description_fini (description_t* d);

ano_t
description_name_to_number (description_t* d,
			    const char* action_name,
			    size_t size);

size_t
description_action_count (description_t* d);

int
description_read (description_t* d,
		  action_desc_t* ad);

#endif /* DESCRIPTION_H */
