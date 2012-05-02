#include "system_msg.h"

int
system_msg_type_write (buffer_file_t* bf,
		       system_msg_type_t type)
{
  return buffer_file_write (bf, &type, sizeof (system_msg_type_t));
}

int
system_msg_type_read (buffer_file_t* bf,
		      system_msg_type_t* type)
{
  return buffer_file_read (bf, type, sizeof (system_msg_type_t));
}

/* typedef struct { */
/*   size_t text_begin; */
/*   size_t text_end; */
/*   size_t bda_begin; */
/*   size_t bda_end; */
/*   size_t bdb_begin; */
/*   size_t bdb_end; */
/*   int retain_privilege; */
/* } create_msg_t; */

/* void */
/* create_initr (create_t* c, */
/* 	      bd_t bda, */
/* 	      bd_t bdb) */
/* { */
/*   buffer_file_t bf; */
/*   buffer_file_initr (&bf, bda); */
/*   const create_msg_t* m = buffer_file_readp (&bf, sizeof (create_msg_t)); */

/*   c->text = buffer_create (0); */
/*   buffer_assign (c->text, bdb, m->text_begin, m->text_end); */

/*   if (m->bda_begin != m->bda_end) { */
/*     c->bda = buffer_create (0); */
/*     buffer_assign (c->bda, bdb, m->bda_begin, m->bda_end); */
/*   } */
/*   else { */
/*     c->bda = -1; */
/*   } */

/*   if (m->bdb_begin != m->bdb_end) { */
/*     c->bdb = buffer_create (0); */
/*     buffer_assign (c->bdb, bdb, m->bdb_begin, m->bdb_end); */
/*   } */
/*   else { */
/*     c->bdb = -1; */
/*   } */

/*   c->retain_privilege = m->retain_privilege; */
/* } */

/* void */
/* create_fini (create_t* c) */
/* { */
/*   if (c->text != -1) { */
/*     buffer_destroy (c->text); */
/*   } */
/*   if (c->bda != -1) { */
/*     buffer_destroy (c->bda); */
/*   } */
/*   if (c->bdb != -1) { */
/*     buffer_destroy (c->bdb); */
/*   } */
/* } */

int
bind_init (bind_t* bind,
	   aid_t output_aid,
	   ano_t output_ano,
	   int output_parameter,
	   aid_t input_aid,
	   ano_t input_ano,
	   int input_parameter)
{
  bind->output_aid = output_aid;
  bind->output_ano = output_ano;
  bind->output_parameter = output_parameter;
  bind->input_aid = input_aid;
  bind->input_ano = input_ano;
  bind->input_parameter = input_parameter;

  return 0;
}

int
bind_write (buffer_file_t* bf,
	    bind_t* bind)
{
  return buffer_file_write (bf, bind, sizeof (bind_t));
}

int
bind_read (buffer_file_t* bf,
	   bind_t* bind)
{
  return buffer_file_read (bf, bind, sizeof (bind_t));
}

int
bind_fini (bind_t* b)
{
  return 0;
}

/* void */
/* unbind_initr (unbind_t* u, */
/* 	      bd_t bd) */
/* { */
/*   buffer_file_t bf; */
/*   buffer_file_initr (&bf, bd); */
/*   buffer_file_read (&bf, u, sizeof (unbind_t)); */
/* } */

/* void */
/* unbind_fini (unbind_t* u) */
/* { */

/* } */

/* void */
/* destroy_initr (destroy_t* d, */
/* 	       bd_t bd) */
/* { */
/*   buffer_file_t bf; */
/*   buffer_file_initr (&bf, bd); */
/*   buffer_file_read (&bf, d, sizeof (destroy_t)); */
/* } */

/* void */
/* destroy_fini (destroy_t* d) */
/* { */

/* } */

int
sysresult_init (sysresult_t* r,
		int retval,
		lily_error_t error)
{
  r->retval = retval;
  r->error = error;
  return 0;
}

int
sysresult_write (buffer_file_t* bf,
		 sysresult_t* r)
{
  return buffer_file_write (bf, r, sizeof (sysresult_t));
}

int
sysresult_fini (sysresult_t* r)
{
  return 0;
}
