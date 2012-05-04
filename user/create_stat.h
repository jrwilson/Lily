#ifndef CREATE_STAT_H
#define CREATE_STAT_H

#include <lily/types.h>
#include <stdbool.h>

typedef struct create_result create_result_t;
typedef struct {
  create_result_t* owner_head;
} create_stat_t;

void
create_stat_init (create_stat_t* bs);

void
create_stat_result (create_stat_t* bs,
		    bd_t bda,
		    bd_t bdb);

#endif /* CREATE_STAT_H */
