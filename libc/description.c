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
  if (buffer_file_initr (&d->bf, bd) != 0) {
    return -1;
  }
  return 0;
}

int
description_fini (description_t* d)
{
  return buffer_destroy (buffer_file_bd (&d->bf));
}

size_t
description_action_count (description_t* d)
{
  if (buffer_file_seek (&d->bf, 0) != 0) {
    return -1;
  }

  size_t count;
  if (buffer_file_read (&d->bf, &count, sizeof (size_t)) != 0) {
    return -1;
  }

  return count;
}

int
description_read_all (description_t* d,
		      action_desc_t* a)
{
  if (buffer_file_seek (&d->bf, 0) != 0) {
    return -1;
  }

  size_t count;
  if (buffer_file_read (&d->bf, &count, sizeof (size_t)) != 0) {
    return -1;
  }

  for (size_t idx = 0; idx != count; ++idx) {
    const action_descriptor_t* ad = buffer_file_readp (&d->bf, sizeof (action_descriptor_t));
    if (ad == 0) {
      return -1;
    }

    const char* name = buffer_file_readp (&d->bf, ad->name_size);
    if (name == 0) {
      return -1;
    }

    const char* description = buffer_file_readp (&d->bf, ad->description_size);
    if (description == 0) {
      return -1;
    }

    a[idx].type = ad->type;
    a[idx].parameter_mode = ad->parameter_mode;
    a[idx].number = ad->number;
    a[idx].name_begin = name;
    a[idx].name_end = name + ad->name_size;
    a[idx].description_begin = description;
    a[idx].description_end = description + ad->description_size;
  }

  return 0;
}

int
description_read_name (description_t* d,
		       action_desc_t* a,
		       const char* name_begin,
		       const char* name_end)
{
  if (name_end == 0) {
    name_end = name_begin + strlen (name_begin);
  }

  if (buffer_file_seek (&d->bf, 0) != 0) {
    return -1;
  }

  size_t count;
  if (buffer_file_read (&d->bf, &count, sizeof (size_t)) != 0) {
    return -1;
  }

  for (size_t idx = 0; idx != count; ++idx) {
    const action_descriptor_t* ad = buffer_file_readp (&d->bf, sizeof (action_descriptor_t));
    if (ad == 0) {
      return -1;
    }

    const char* name = buffer_file_readp (&d->bf, ad->name_size);
    if (name == 0) {
      return -1;
    }

    const char* description = buffer_file_readp (&d->bf, ad->description_size);
    if (description == 0) {
      return -1;
    }

    if (pstrcmp (name, 0, name_begin, name_end) == 0) {
      a->type = ad->type;
      a->parameter_mode = ad->parameter_mode;
      a->number = ad->number;
      a->name_begin = name;
      a->name_end = name + ad->name_size;
      a->description_begin = description;
      a->description_end = description + ad->description_size;
      return 0;
    }
  }

  return -1;
}
