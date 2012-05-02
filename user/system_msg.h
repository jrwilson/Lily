#ifndef SYSTEM_MSG_H
#define SYSTEM_MSG_H

#include <automaton.h>
#include <buffer_file.h>

typedef enum {
  CREATE,
  BIND,
  UNBIND,
  DESTROY,
} system_msg_type_t;

int
system_msg_type_write (buffer_file_t* bf,
		       system_msg_type_t type);

int
system_msg_type_read (buffer_file_t* bf,
		      system_msg_type_t* type);

/* typedef struct { */
/*   bd_t text; */
/*   bd_t bda; */
/*   bd_t bdb; */
/*   int retain_privilege; */
/* } create_t; */

/* void */
/* create_initr (create_t* c, */
/* 	      bd_t bda, */
/* 	      bd_t bdb); */

/* void */
/* create_fini (create_t* c); */

typedef struct {
  aid_t output_aid;
  ano_t output_ano;
  int output_parameter;
  aid_t input_aid;
  ano_t input_ano;
  int input_parameter;
} bind_t;

int
bind_init (bind_t* bind,
	   aid_t output_aid,
	   ano_t output_ano,
	   int output_parameter,
	   aid_t input_aid,
	   ano_t input_ano,
	   int input_parameter);

int
bind_write (buffer_file_t* bf,
	    bind_t* bind);

int
bind_read (buffer_file_t* bf,
	   bind_t* bind);

int
bind_fini (bind_t* b);

/* typedef struct { */
/*   bid_t bid; */
/* } unbind_t; */

/* void */
/* unbind_initr (unbind_t* u, */
/* 	      bd_t bd); */

/* void */
/* unbind_fini (unbind_t* u); */

/* typedef struct { */
/*   aid_t aid; */
/* } destroy_t; */

/* void */
/* destroy_initr (destroy_t* d, */
/* 	       bd_t bd); */

/* void */
/* destroy_fini (destroy_t* d); */

typedef struct {
  int retval;
  lily_error_t error;
} sysresult_t;

int
sysresult_init (sysresult_t* r,
		int retval,
		lily_error_t error);

int
sysresult_write (buffer_file_t* bf,
		 sysresult_t* r);

int
sysresult_fini (sysresult_t* r);

#endif /* SYSTEM_MSG_H */
