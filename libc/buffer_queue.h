#ifndef BUFFER_QUEUE_H
#define BUFFER_QUEUE_H

#include <stdbool.h>
#include <lily/types.h>

typedef struct buffer_queue_item_struct buffer_queue_item_t;

typedef struct {
  buffer_queue_item_t* head;
  buffer_queue_item_t** tail;
} buffer_queue_t;

void
buffer_queue_init (buffer_queue_t* bq);

void
buffer_queue_push (buffer_queue_t* bq,
		   int parameter,
		   bd_t bd);

void
buffer_queue_pop (buffer_queue_t* bq);

buffer_queue_item_t*
buffer_queue_find (const buffer_queue_t* bq,
		   int parameter);

void
buffer_queue_erase (buffer_queue_t* bq,
		    const buffer_queue_item_t* item);

bool
buffer_queue_empty (const buffer_queue_t* bq);

buffer_queue_item_t*
buffer_queue_front (const buffer_queue_t* bq);

int
buffer_queue_item_parameter (const buffer_queue_item_t* item);

bd_t
buffer_queue_item_bd (const buffer_queue_item_t* item);

#endif /* BUFFER_QUEUE_H */
