#include "data_stack.h"
#include <string.h>
#include <dymem.h>

typedef enum {
  PUSH_TABLE,
  PUSH_STRING,
  PUSH_INTEGER,
  INSERT,
} stack_op_t;

int
data_stack_initw (data_stack_t* ds,
		  bd_t bd)
{
  if (buffer_file_initw (&ds->bf, bd) != 0) {
    return -1;
  }

  return 0;
}

int
data_stack_push_table (data_stack_t* ds)
{
  stack_op_t op = PUSH_TABLE;
  if (buffer_file_write (&ds->bf, &op, sizeof (stack_op_t)) != 0) {
    return -1;
  }
  return 0;
}

int
data_stack_push_string (data_stack_t* ds,
			const char* str)
{
  stack_op_t op = PUSH_STRING;
  size_t len = strlen (str) + 1; // Include terminator.
  if (buffer_file_write (&ds->bf, &op, sizeof (stack_op_t)) != 0 ||
      buffer_file_write (&ds->bf, &len, sizeof (size_t)) != 0 ||
      buffer_file_write (&ds->bf, str, len) != 0) {
    return -1;
  }
  return 0;
}

int
data_stack_push_integer (data_stack_t* ds,
			 int i)
{
  stack_op_t op = PUSH_INTEGER;
  if (buffer_file_write (&ds->bf, &op, sizeof (stack_op_t)) != 0 ||
      buffer_file_write (&ds->bf, &i, sizeof (int)) != 0) {
    return -1;
  }
  return 0;
}

int
data_stack_insert (data_stack_t* ds)
{
  stack_op_t op = INSERT;
  if (buffer_file_write (&ds->bf, &op, sizeof (stack_op_t)) != 0) {
    return -1;
  }
  return 0;
}

static object_t*
create_table (void)
{
  object_t* obj = malloc (sizeof (object_t));
  memset (obj, 0, sizeof (object_t));
  obj->type = TABLE;
  return obj;
}

static object_t*
create_string (char* str,
	       size_t size)
{
  object_t* obj = malloc (sizeof (object_t));
  memset (obj, 0, sizeof (object_t));
  obj->type = STRING;
  obj->u.string.str = str;
  obj->u.string.size = size;
  return obj;
}

static object_t*
create_integer (int i)
{
  object_t* obj = malloc (sizeof (object_t));
  memset (obj, 0, sizeof (object_t));
  obj->type = INTEGER;
  obj->u.integer.i = i;
  return obj;
}

static void
destroy_object (object_t* obj)
{
  switch (obj->type) {
  case TABLE:
    {
      while (obj->u.table.head != 0) {
	pair_t* p = obj->u.table.head;
	obj->u.table.head = p->next;
	destroy_object (p->key);
	destroy_object (p->value);
	free (p);
      }
    }
    break;
  case STRING:
    free (obj->u.string.str);
    break;
  case INTEGER:
    break;
  }
  free (obj);
}

static object_t*
at (data_stack_t* ds,
    int offset)
{
  return ds->stack[ds->stack_size + offset];
}

static void
push (data_stack_t* ds,
      object_t* obj)
{
  if (ds->stack_size == ds->stack_capacity) {
    ds->stack_capacity = 2 * ds->stack_capacity + 1;
    ds->stack = realloc (ds->stack, ds->stack_capacity * sizeof (object_t*));
  }
  ds->stack[ds->stack_size++] = obj;
}

static bool
equal (object_t* x,
       object_t* y)
{
  if (x->type == y->type) {
    switch (x->type) {
    case TABLE:
      return false;
    case STRING:
      return x->u.string.size == y->u.string.size && memcmp (x->u.string.str, y->u.string.str, y->u.string.size) == 0;
    case INTEGER:
      return x->u.integer.i == y->u.integer.i;
    }
  }

  return false;
}

static void
insert (object_t* table,
	object_t* key,
	object_t* value)
{
  pair_t* p;
  for (p = table->u.table.head; p != 0; p = p->next) {
    if (equal (p->key, key)) {
      break;
    }
  }

  if (p == 0) {
    p = malloc (sizeof (pair_t));
    p->key = key;
    p->value = value;
    p->next = table->u.table.head;
    table->u.table.head = p;
  }
  else {
    destroy_object (p->key);
    destroy_object (p->value);
    p->key = key;
    p->value = value;
  }
}

int
data_stack_initr (data_stack_t* ds,
		  bd_t bd)
{
  if (buffer_file_initr (&ds->bf, bd) != 0) {
    return -1;
  }

  ds->stack = 0;
  ds->stack_size = 0;
  ds->stack_capacity = 0;

  for (;;) {
    stack_op_t op;
    if (buffer_file_read (&ds->bf, &op, sizeof (stack_op_t)) != 0) {
      return 0;
    }
    switch (op) {
    case PUSH_TABLE:
      push (ds, create_table ());
      break;
    case PUSH_STRING:
      {
	size_t len;
	if (buffer_file_read (&ds->bf, &len, sizeof (size_t)) != 0) {
	  return -1;
	}
	char* str = malloc (len);
	if (buffer_file_read (&ds->bf, str, len) != 0) {
	  free (str);
	  return -1;
	}
	push (ds, create_string (str, len));
      }
      break;
    case PUSH_INTEGER:
      {
	int i;
	if (buffer_file_read (&ds->bf, &i, sizeof (int)) != 0) {
	  return -1;
	}
	push (ds, create_integer (i));
      }
      break;
    case INSERT:
      {
	if (ds->stack_size < 3) {
	  return -1;
	}
	object_t* value = at (ds, -1);
	object_t* key = at (ds, -2);
	object_t* table = at (ds, -3);

	if (table->type != TABLE) {
	  return -1;
	}
	insert (table, key, value);
	ds->stack_size -= 2;
      }
      break;
    default:
      return -1;
    }
  }

  return 0;
}
