#ifndef CALLBACK_QUEUE_H
#define CALLBACK_QUEUE_H

#include <stdbool.h>
#include <lily/types.h>

typedef struct callback_queue_item_struct callback_queue_item_t;

typedef struct {
  callback_queue_item_t* head;
  callback_queue_item_t** tail;
} callback_queue_t;

typedef void (*callback_t) (void*, bd_t, bd_t);

void
callback_queue_init (callback_queue_t* bq);

void
callback_queue_push (callback_queue_t* bq,
		     callback_t callback,
		     void* data);

void
callback_queue_pop (callback_queue_t* bq);

bool
callback_queue_empty (const callback_queue_t* bq);

const callback_queue_item_t*
callback_queue_front (const callback_queue_t* bq);

callback_t
callback_queue_item_callback (const callback_queue_item_t* item);

void*
callback_queue_item_data (const callback_queue_item_t* item);

#endif /* CALLBACK_QUEUE_H */
