#ifndef FINDA_H
#define FINDA_H

#include <automaton.h>
#include "constellation.h"

#define FINDA_RECV_NAME "finda_recv_out"
#define FINDA_SEND_NAME "finda_send_in"

typedef struct {
  
} finda_t;

typedef void (*finda_callback_t) (void* arg);

void
finda_init (finda_t* finda,
	    constellation_t* constellation,
	    aid_t aid,
	    ano_t send,
	    ano_t recv);

void
finda_send (finda_t* finda);

void
finda_recv (finda_t* finda,
	    bd_t bda,
	    bd_t bdb);

void
finda_schedule (finda_t* finda);

void
finda_find (finda_t* finda,
	    const char* name_begin,
	    const char* name_end,
	    finda_callback_t callback,
	    void* arg);

#endif /* FINDA_H */
