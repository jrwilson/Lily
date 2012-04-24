#ifndef DE_H
#define DE_H

#include <buffer_file.h>

typedef enum {
  DE_STRING,
  DE_INTEGER,
  DE_OBJECT,
  DE_ARRAY,
  DE_NULL,

  DE_UNKNOWN,
} de_val_type_t;

typedef struct de_val de_val_t;
typedef struct de_pair de_pair_t;

de_val_type_t
de_type (const de_val_t* val);

de_val_t*
de_create_string (const char* str);

const char*
de_string_val (const de_val_t* val,
	       const char* def);

de_val_t*
de_create_integer (int i);

int
de_integer_val (const de_val_t* val,
		int def);

de_val_t*
de_create_object (void);

de_pair_t*
de_object_begin (const de_val_t* val);

de_val_t*
de_pair_key (const de_pair_t* pair);

de_val_t*
de_pair_value (const de_pair_t* pair);

de_pair_t*
de_pair_next (const de_pair_t* pair);

de_val_t*
de_create_array (void);

size_t
de_array_size (const de_val_t* val);

de_val_t*
de_array_at (const de_val_t* val,
	     size_t idx);

void
de_destroy (de_val_t* val);

void
de_set (de_val_t* obj,
	const char* path,
	de_val_t* val);

de_val_t*
de_get (de_val_t* obj,
	const char* path);

void
de_serialize (de_val_t* val,
	      buffer_file_t* bf);

de_val_t*
de_deserialize (buffer_file_t* bf);

#endif /* DE_H */
