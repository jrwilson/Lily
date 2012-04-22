#include "callback_queue.h"
#include "dymem.h"

struct callback_queue_item_struct {
  callback_t callback;
  void* data;
  callback_queue_item_t* next;
};

void
callback_queue_init (callback_queue_t* bq)
{
  bq->head = 0;
  bq->tail = &bq->head;
}

int
callback_queue_push (callback_queue_t* bq,
		     callback_t callback,
		     void* data)
{
  /* Create a queue item. */
  callback_queue_item_t* item = malloc (sizeof (callback_queue_item_t));
  if (item == 0) {
    return -1;
  }
  item->callback = callback;
  item->data = data;
  item->next = 0;

  /* Insert into the queue. */
  *bq->tail = item;
  bq->tail = &item->next;

  return 0;
}

void
callback_queue_pop (callback_queue_t* bq)
{
  callback_queue_item_t* item = bq->head;
  bq->head = item->next;

  free (item);
  
  if (bq->head == 0) {
    bq->tail = &bq->head;
  }
}

bool
callback_queue_empty (const callback_queue_t* bq)
{
  return bq->head == 0;
}

const callback_queue_item_t*
callback_queue_front (const callback_queue_t* bq)
{
  return bq->head;
}

callback_t
callback_queue_item_callback (const callback_queue_item_t* item)
{
  return item->callback;
}

void*
callback_queue_item_data (const callback_queue_item_t* item)
{
  return item->data;
}
