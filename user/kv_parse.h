#ifndef KV_PARSE_H
#define KV_PARSE_H

/* Parse key/value pairs separated by =. */

#include <lily/types.h>

int
kv_parse (lily_error_t* err,
	  char** key,
	  char** value,
	  const char** ptr,
	  const char* end);

#endif /* KV_PARSE_H */
