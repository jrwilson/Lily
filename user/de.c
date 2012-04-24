#include "de.h"
#include <dymem.h>
#include <string.h>
#include <limits.h>

struct de_pair {
  de_val_t* key;
  de_val_t* value;
  de_pair_t* next;
};

struct de_val {
  de_val_type_t type;
  union {
    struct {
      de_pair_t* head;
    } object;
    struct {
      de_val_t** arr;
      size_t size;
      size_t capacity;
    } array;
    struct {
      int i;
    } integer;
    struct {
      char* str;
      size_t size;
    } string;
  } u;
};

static de_val_t NULL_VAL = { .type = DE_NULL };

/* TODO:  Change this to overwrite. */
static de_pair_t*
object_insert (de_val_t* obj,
	       de_val_t* key,
	       de_val_t* value)
{
  de_pair_t* p = malloc (sizeof (de_pair_t));
  p->key = key;
  p->value = value;
  p->next = obj->u.object.head;
  obj->u.object.head = p;
  return p;
}

static void
array_ensure (de_val_t* arr,
	      size_t capacity)
{
  if (arr->u.array.capacity < capacity) {
    size_t new_capacity = capacity * 2;
    arr->u.array.arr = realloc (arr->u.array.arr, new_capacity * sizeof (de_val_t*));
    for (size_t idx = arr->u.array.capacity; idx != new_capacity; ++idx) {
      /* Values point to NULL_VAL by default. */
      arr->u.array.arr[idx] = &NULL_VAL;
    }
    arr->u.array.capacity = new_capacity;
  }

  if (arr->u.array.size < capacity) {
    arr->u.array.size = capacity;
  }
}

static void
convert_to_unknown (de_val_t* val)
{
  switch (val->type) {
  case DE_STRING:
    free (val->u.string.str);
    val->u.string.str = 0;
    val->u.string.size = 0;
    val->type = DE_UNKNOWN;
    break;
  case DE_INTEGER:
    val->u.integer.i = 0;
    val->type = DE_UNKNOWN;
    break;
  case DE_OBJECT:
    while (val->u.object.head != 0) {
      de_pair_t* p = val->u.object.head;
      val->u.object.head = p->next;
      de_destroy (p->key);
      de_destroy (p->value);
      free (p);
    }
    val->type = DE_UNKNOWN;
    break;
  case DE_ARRAY:
    for (size_t idx = 0; idx != val->u.array.size; ++idx) {
      if (val->u.array.arr[idx] != 0) {
	de_destroy (val->u.array.arr[idx]);
      }
    }
    free (val->u.array.arr);
    val->u.array.arr = 0;
    val->u.array.size = 0;
    val->u.array.capacity = 0;
    val->type = DE_UNKNOWN;
    break;
  case DE_NULL:
    /* Nothing. */
    break;
  case DE_UNKNOWN:
    /* Nothing. */
    break;
  }
}

de_val_type_t
de_type (const de_val_t* val)
{
  return val->type;
}

de_val_t*
de_create_object (void)
{
  de_val_t* obj = malloc (sizeof (de_val_t));
  memset (obj, 0, sizeof (de_val_t));
  obj->type = DE_OBJECT;
  return obj;
}

de_val_t*
de_create_array (void)
{
  de_val_t* obj = malloc (sizeof (de_val_t));
  memset (obj, 0, sizeof (de_val_t));
  obj->type = DE_ARRAY;
  return obj;
}

size_t
de_array_size (const de_val_t* val)
{
  return val->u.array.size;
}

de_val_t*
de_array_at (const de_val_t* val,
	     size_t idx)
{
  /* No bounds check. */
  return val->u.array.arr[idx];
}

de_val_t*
de_create_integer (int i)
{
  de_val_t* obj = malloc (sizeof (de_val_t));
  memset (obj, 0, sizeof (de_val_t));
  obj->type = DE_INTEGER;
  obj->u.integer.i = i;
  return obj;
}

int
de_integer_val (const de_val_t* val,
		int def)
{
  if (val->type == DE_INTEGER) {
    return val->u.integer.i;
  }
  else {
    return def;
  }
}

static de_val_t*
create_string (const char* begin,
	       const char* end)
{
  size_t len = end - begin;
  de_val_t* obj = malloc (sizeof (de_val_t));
  memset (obj, 0, sizeof (de_val_t));
  obj->type = DE_STRING;
  obj->u.string.str = malloc (len + 1);
  obj->u.string.size = len + 1;
  memcpy (obj->u.string.str, begin, len);
  obj->u.string.str[len] = '\0';
  return obj;
}

const char*
de_string_val (const de_val_t* val,
	       const char* def)
{
  if (val->type == DE_STRING) {
    return val->u.string.str;
  }
  else {
    return def;
  }
}

de_val_t*
de_create_string (const char* str)
{
  size_t size = strlen (str) + 1;
  de_val_t* obj = malloc (sizeof (de_val_t));
  memset (obj, 0, sizeof (de_val_t));
  obj->type = DE_STRING;
  obj->u.string.str = malloc (size);
  obj->u.string.size = size;
  memcpy (obj->u.string.str, str, size);
  return obj;
}

static de_val_t*
create_unknown (void)
{
  de_val_t* obj = malloc (sizeof (de_val_t));
  memset (obj, 0, sizeof (de_val_t));
  obj->type = DE_UNKNOWN;
  return obj;
}

void
de_destroy (de_val_t* val)
{
  if (val->type != DE_NULL) {
    convert_to_unknown (val);
    free (val);
  }
}

de_pair_t*
de_object_begin (const de_val_t* val)
{
  return val->u.object.head;
}

de_val_t*
de_pair_key (const de_pair_t* pair)
{
  return pair->key;
}

de_val_t*
de_pair_value (const de_pair_t* pair)
{
  return pair->value;
}

de_pair_t*
de_pair_next (const de_pair_t* pair)
{
  return pair->next;
}

static de_val_t**
get_lhs (de_val_t* obj,
	 const char* path)
{
  if (*path == '.') {
    if (obj->type != DE_OBJECT) {
      /* Incorrect type. */
      convert_to_unknown (obj);
      obj->type = DE_OBJECT;
    }
    /* Advance past the '.' */
    ++path;
    /* Find the boundaries of the member name. */
    const char* begin = path;
    const char* end = begin;
    while (*end != '\0' && *end != '.' && *end != '[') {
      ++end;
    }

    if (begin == end) {
      /* No member. */
      return 0;
    }

    /* Find the member. */
    size_t size = end - begin + 1;
    de_pair_t* p;
    for (p = obj->u.object.head; p != 0; p = p->next) {
      if (p->key->u.string.size == size && strncmp (p->key->u.string.str, begin, size - 1) == 0) {
	break;
      }
    }

    if (p == 0) {
      /* Create the member if necessary. */
      p = object_insert (obj, create_string (begin, end), &NULL_VAL);
    }
    
    if (*end != '\0') {
      if (p->value->type == DE_NULL) {
	p->value = create_unknown ();
      }
      return get_lhs (p->value, end);
    }
    else {
      return &p->value;
    }
  }
  else if (*path == '[') {
    if (obj->type != DE_ARRAY) {
      /* Incorrect type. */
      convert_to_unknown (obj);
      obj->type = DE_ARRAY;
    }
    /* Advance past the '[' */
    ++path;
    char* end;
    size_t idx = strtoul (path, &end, 0);
    if (string_error != STRING_SUCCESS) {
      /* Bad index. */
      return 0;
    }
    
    if (*end != ']') {
      /* Expected ']'. */
      return 0;
    }
    ++end;
    
    if (idx > UINT_MAX) {
      /* Out of range. */
      return 0;
    }

    /* Ensure the array has enough space. */
    array_ensure (obj, idx + 1);

    de_val_t** r = &obj->u.array.arr[idx];

    if (*end != '\0') {
      if ((*r)->type == DE_NULL) {
	*r = create_unknown ();
      }
      return get_lhs (*r, end);
    }
    else {
      return r;
    }
  }
  else {
    /* Bad path. */
    return 0;
  }
}

void
de_set (de_val_t* obj,
	const char* path,
	de_val_t* val)
{
  de_val_t** lhs = get_lhs (obj, path);
  if (lhs != 0) {
    if (*lhs != 0) {
      de_destroy (*lhs);
    }
    *lhs = val;
  }
}

de_val_t*
de_get (de_val_t* obj,
	const char* path)
{
  de_val_t** lhs = get_lhs (obj, path);
  if (lhs != 0) {
    return *lhs;
  }
  else {
    return &NULL_VAL;
  }
}

typedef enum {
  OP_STRING,
  OP_INTEGER,
  OP_NULL,
  BEGIN_OBJECT,
  END_OBJECT,
  BEGIN_ARRAY,
  END_ARRAY,
} stack_op_t;

void
de_serialize (de_val_t* val,
	      buffer_file_t* bf)
{
  switch (val->type) {
  case DE_STRING:
    {
      stack_op_t op = OP_STRING;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
      buffer_file_write (bf, &val->u.string.size, sizeof (size_t));
      buffer_file_write (bf, val->u.string.str, val->u.string.size);
    }
    break;
  case DE_INTEGER:
    {
      stack_op_t op = OP_INTEGER;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
      buffer_file_write (bf, &val->u.integer.i, sizeof (int));
    }
    break;
  case DE_OBJECT:
    {
      stack_op_t op = BEGIN_OBJECT;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
      for (de_pair_t* p = val->u.object.head; p != 0; p = p->next) {
	de_serialize (p->key, bf);
	de_serialize (p->value, bf);
      }
      op = END_OBJECT;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
    }
    break;
  case DE_ARRAY:
    {
      stack_op_t op = BEGIN_ARRAY;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
      op = OP_NULL;
      for (size_t idx = 0; idx != val->u.array.size; ++idx) {
	if (val->u.array.arr[idx] != 0) {
	  de_serialize (val->u.array.arr[idx], bf);
	}
	else {
	  buffer_file_write (bf, &op, sizeof (stack_op_t));
	}
      }
      op = END_ARRAY;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
    }
    break;
  case DE_NULL:
  case DE_UNKNOWN:
    {
      stack_op_t op = OP_NULL;
      buffer_file_write (bf, &op, sizeof (stack_op_t));
    }
    break;
  }
}

typedef struct context context_t;
struct context {
  de_val_t* val;
  de_val_t* key;
  context_t* next;
};

static void
context_push (context_t* context,
	      de_val_t* val)
{
  switch (context->val->type) {
  case DE_OBJECT:
    if (context->key == 0) {
      context->key = val;
    }
    else {
      object_insert (context->val, context->key, val);
      context->key = 0;
    }
    break;
  case DE_ARRAY:
    {
      size_t idx = context->val->u.array.size;
      array_ensure (context->val, idx + 1);
      context->val->u.array.arr[idx] = val;
    }
    break;
  default:
    /* Error. */
    break;
  }
}

de_val_t*
de_deserialize (buffer_file_t* bf)
{
  context_t* top = 0;

  stack_op_t op;
  while (buffer_file_read (bf, &op, sizeof (stack_op_t)) == 0) {
    switch (op) {
    case OP_STRING:
      {
	size_t size;
	buffer_file_read (bf, &size, sizeof (size_t));
	const char* str = buffer_file_readp (bf, size);
	de_val_t* string = create_string (str, str + size - 1);
	if (top != 0) {
	  context_push (top, string);
	}
	else {
	  /* No context.  Done. */
	  return string;
	}
      }
      break;
    case OP_INTEGER:
      {
	int i;
	buffer_file_read (bf, &i, sizeof (int));
	de_val_t* integer = de_create_integer (i);
	if (top != 0) {
	  context_push (top, integer);
	}
	else {
	  /* No context.  Done. */
	  return integer;
	}
      }
      break;
    case OP_NULL:
      {
	if (top != 0) {
	  context_push (top, &NULL_VAL);
	}
	else {
	  /* No context.  Done. */
	  return &NULL_VAL;
	}
      }
      break;
    case BEGIN_OBJECT:
      {
	context_t* c = malloc (sizeof (context_t));
	memset (c, 0, sizeof (context_t));
	c->val = de_create_object ();
	c->next = top;
	top = c;
      }
      break;
    case BEGIN_ARRAY:
      {
	context_t* c = malloc (sizeof (context_t));
	memset (c, 0, sizeof (context_t));
	c->val = de_create_array ();
	c->next = top;
	top = c;
      }
      break;
    case END_OBJECT:
    case END_ARRAY:
      {
	de_val_t* object = top->val;
	context_t* c = top;
	top = c->next;
	free (c);

	if (top != 0) {
	  context_push (top, object);
	}
	else {
	  /* No context. Done. */
	  return object;
	}
      }
      break;
    }
  }

  return 0;
}
