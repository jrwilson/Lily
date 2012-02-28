#include "buffer_queue.h"
#include "dymem.h"


struct buffer_queue_item_struct {
  int parameter;
  bd_t bda;
  size_t sizea;
  bd_t bdb;
  size_t sizeb;
  buffer_queue_item_t* next;
};

void
buffer_queue_init (buffer_queue_t* bq)
{
  bq->head = 0;
  bq->tail = &bq->head;
}

void
buffer_queue_push (buffer_queue_t* bq,
		   int parameter,
		   bd_t bda,
		   size_t sizea,
		   bd_t bdb,
		   size_t sizeb)
{
  /* Create a queue item. */
  buffer_queue_item_t* item = malloc (sizeof (buffer_queue_item_t));
  item->parameter = parameter;
  item->bda = bda;
  item->sizea = sizea;
  item->bdb = bdb;
  item->sizeb = sizeb;
  item->next = 0;

  /* Insert into the queue. */
  *bq->tail = item;
  bq->tail = &item->next;
}

void
buffer_queue_pop (buffer_queue_t* bq)
{
  buffer_queue_item_t* item = bq->head;
  bq->head = item->next;

  free (item);
  
  if (bq->head == 0) {
    bq->tail = &bq->head;
  }
}

buffer_queue_item_t*
buffer_queue_find (const buffer_queue_t* bq,
		   int parameter)
{
  buffer_queue_item_t* item;
  for (item = bq->head; item != 0; item = item->next) {
    if (item->parameter == parameter) {
      break;
    }
  }

  return item;
}

void
buffer_queue_erase (buffer_queue_t* bq,
		    const buffer_queue_item_t* item)
{
  buffer_queue_item_t** ptr;
  for (ptr = &bq->head; *ptr != 0; ptr = &(*ptr)->next) {
    if (*ptr == item) {
      break;
    }
  }

  if (*ptr != 0) {
    buffer_queue_item_t* tmp = *ptr;
    *ptr = tmp->next;
    free (tmp);
    if (bq->head == 0) {
      bq->tail = &bq->head;
    }
  }
}

bool
buffer_queue_empty (const buffer_queue_t* bq)
{
  return bq->head == 0;
}

const buffer_queue_item_t*
buffer_queue_front (const buffer_queue_t* bq)
{
  return bq->head;
}

int
buffer_queue_item_parameter (const buffer_queue_item_t* item)
{
  return item->parameter;
}

bd_t
buffer_queue_item_bda (const buffer_queue_item_t* item)
{
  return item->bda;
}

size_t
buffer_queue_item_sizea (const buffer_queue_item_t* item)
{
  return item->sizea;
}

bd_t
buffer_queue_item_bdb (const buffer_queue_item_t* item)
{
  return item->bdb;
}

size_t
buffer_queue_item_sizeb (const buffer_queue_item_t* item)
{
  return item->sizeb;
}
