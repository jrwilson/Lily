#include "automaton.h"
#include "buffer_file.h"
#include "string.h"

ano_t
action_name_to_number (bd_t bd,
		       size_t bd_size,
		       void* ptr,
		       const char* action_name)
{
  buffer_file_t bf;
  buffer_file_open (&bf, bd, bd_size, ptr, false);
  const size_t* count = buffer_file_readp (&bf, sizeof (size_t));
  if (count == 0) {
    return NO_ACTION;
  }

  for (size_t idx = 0; idx != *count; ++idx) {
    const action_descriptor_t* d = buffer_file_readp (&bf, sizeof (action_descriptor_t));
    if (d == 0) {
      return NO_ACTION;
    }

    const char* name = buffer_file_readp (&bf, d->name_size);
    if (name == 0) {
      return NO_ACTION;
    }

    const char* description = buffer_file_readp (&bf, d->description_size);
    if (description == 0) {
      return NO_ACTION;
    }

    if (strcmp (name, action_name) == 0) {
      return d->number;
    }
  }

  return NO_ACTION;
}
