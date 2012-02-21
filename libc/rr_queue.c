#include "rr_queue.h"
#include "dymem.h"
#include "string.h"

struct rr_queue_item_struct {
  bd_t bd;
  size_t bd_size;
  void (*handler) (void*, int, bd_t, size_t);
  void* data;
  rr_queue_item_t* next;
};

void
rr_queue_init (rr_queue_t* rr)
{
  rr->request_queue_head = 0;
  rr->request_queue_tail = &rr->request_queue_head;
  rr->handler_queue_head = 0;
  rr->handler_queue_tail = &rr->handler_queue_head;
}

bool
rr_queue_request_empty (const rr_queue_t* rr)
{
  return rr->request_queue_head == 0;
}

bool
rr_queue_handler_empty (const rr_queue_t* rr)
{
  return rr->handler_queue_head == 0;
}

const rr_queue_item_t*
rr_queue_request_front (const rr_queue_t* rr)
{
  return rr->request_queue_head;
}

const rr_queue_item_t*
rr_queue_handler_front (const rr_queue_t* rr)
{
  return rr->handler_queue_head;
}

void
rr_queue_push (rr_queue_t* rr,
	       bd_t bd,
	       size_t bd_size,
	       void (*handler) (void*, int, bd_t, size_t),
	       void* data)
{
  rr_queue_item_t* item = malloc (sizeof (rr_queue_item_t*));
  memset (item, 0, sizeof (rr_queue_item_t));
  item->bd = bd;
  item->bd_size = bd_size;
  item->handler = handler;
  item->data = data;
  *rr->request_queue_tail = item;
  rr->request_queue_tail = &item->next;
}

void
rr_queue_shift (rr_queue_t* rr)
{
  rr_queue_item_t* item = rr->request_queue_head;
  rr->request_queue_head = item->next;
  if (rr->request_queue_head == 0) {
    rr->request_queue_tail = &rr->request_queue_head;
  }

  item->next = 0;

  *rr->handler_queue_tail = item;
  rr->handler_queue_tail = &item->next;
}

void
rr_queue_pop (rr_queue_t* rr)
{
  rr_queue_item_t* item = rr->handler_queue_head;
  rr->handler_queue_head = item->next;
  if (rr->handler_queue_head == 0) {
    rr->handler_queue_tail = & rr->handler_queue_head;
  }

  free (item);
}

bd_t
rr_queue_item_bd (const rr_queue_item_t* item)
{
  return item->bd;
}

size_t
rr_queue_item_size (const rr_queue_item_t* item)
{
  return item->bd_size;
}

void*
rr_queue_item_handler (const rr_queue_item_t* item)
{
  return item->handler;
}

void*
rr_queue_item_data (const rr_queue_item_t* item)
{
  return item->data;
}
