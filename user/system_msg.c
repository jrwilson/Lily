#include "system_msg.h"
#include <buffer_file.h>

/* TODO:  Error checking. */

typedef struct {
  size_t text_begin;
  size_t text_end;
  size_t bda_begin;
  size_t bda_end;
  size_t bdb_begin;
  size_t bdb_end;
  int retain_privilege;
} create_msg_t;

void
create_initr (create_t* c,
	      bd_t bda,
	      bd_t bdb)
{
  buffer_file_t bf;
  buffer_file_initr (&bf, bda);
  const create_msg_t* m = buffer_file_readp (&bf, sizeof (create_msg_t));

  c->text = buffer_create (0);
  buffer_assign (c->text, bdb, m->text_begin, m->text_end);

  if (m->bda_begin != m->bda_end) {
    c->bda = buffer_create (0);
    buffer_assign (c->bda, bdb, m->bda_begin, m->bda_end);
  }
  else {
    c->bda = -1;
  }

  if (m->bdb_begin != m->bdb_end) {
    c->bdb = buffer_create (0);
    buffer_assign (c->bdb, bdb, m->bdb_begin, m->bdb_end);
  }
  else {
    c->bdb = -1;
  }

  c->retain_privilege = m->retain_privilege;
}

void
create_fini (create_t* c)
{
  if (c->text != -1) {
    buffer_destroy (c->text);
  }
  if (c->bda != -1) {
    buffer_destroy (c->bda);
  }
  if (c->bdb != -1) {
    buffer_destroy (c->bdb);
  }
}

void
bind_initr (bind_t* b,
	    bd_t bd)
{
  buffer_file_t bf;
  buffer_file_initr (&bf, bd);
  buffer_file_read (&bf, b, sizeof (bind_t));
}

void
bind_fini (bind_t* b)
{

}

void
unbind_initr (unbind_t* u,
	      bd_t bd)
{
  buffer_file_t bf;
  buffer_file_initr (&bf, bd);
  buffer_file_read (&bf, u, sizeof (unbind_t));
}

void
unbind_fini (unbind_t* u)
{

}

void
destroy_initr (destroy_t* d,
	       bd_t bd)
{
  buffer_file_t bf;
  buffer_file_initr (&bf, bd);
  buffer_file_read (&bf, d, sizeof (destroy_t));
}

void
destroy_fini (destroy_t* d)
{

}
