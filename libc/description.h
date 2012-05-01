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

int
description_fini (description_t* d);

size_t
description_action_count (description_t* d);

int
description_read_all (description_t* d,
		      action_desc_t* ad);

int
description_read_name (description_t* d,
		       action_desc_t* ad,
		       const char* name_begin,
		       const char* name_end);

#endif /* DESCRIPTION_H */
