#include "description.h"
#include "automaton.h"
#include "buffer_file.h"
#include "string.h"

int
description_init (description_t* d,
		  aid_t aid)
{
  d->aid = aid;
  bd_t bd = describe (aid);
  if (bd == -1) {
    return -1;
  }
  size_t bd_size = buffer_size (bd);
  buffer_file_open (&d->bf, bd, bd_size, false);
  return 0;
}

void
description_fini (description_t* d)
{
  buffer_destroy (buffer_file_bd (&d->bf));
}

ano_t
description_name_to_number (description_t* d,
			    const char* action_name)
{
  buffer_file_seek (&d->bf, 0, BUFFER_FILE_SET);
  const size_t* count = buffer_file_readp (&d->bf, sizeof (size_t));
  if (count == 0) {
    return NO_ACTION;
  }

  for (size_t idx = 0; idx != *count; ++idx) {
    const action_descriptor_t* ad = buffer_file_readp (&d->bf, sizeof (action_descriptor_t));
    if (ad == 0) {
      return NO_ACTION;
    }

    const char* name = buffer_file_readp (&d->bf, ad->name_size);
    if (name == 0) {
      return NO_ACTION;
    }

    const char* description = buffer_file_readp (&d->bf, ad->description_size);
    if (description == 0) {
      return NO_ACTION;
    }

    if (strcmp (name, action_name) == 0) {
      return ad->number;
    }
  }

  return NO_ACTION;
}
