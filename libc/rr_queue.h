#ifndef RR_QUEUE_H
#define RR_QUEUE_H

#include <stdbool.h>
#include <lily/types.h>

/* Request-Response Queue
   ----------------------
   A request-response is used to facilitate interactions that follow a request-response pattern where:
   1.  All requests generate exactly one response.
   2.  The responses are generated in the order that the requests are received.
   3.  The may be an arbitrary number of outstanding requests.

   A request-response queue works as a pair of queues where one queue contains requests-handler pairs and the other contains handlers.
   In the following scenario, there are five requests waiting to be sent and three handlers are waiting for a response (* marks the head):
     rrrrr*    hhh*
   Suppose the user makes a request.  The request-handler pair is added to the request queue:
    rrrrrr*    hhh*
   Suppose then that a response is received.  The next handler is popped and invoked:
    rrrrrr*    hh*
   Suppose then that a request is sent.  The request-handler pair is popped, the request is sent, and the handler is pushed:
     rrrrr*    hhh*
*/

typedef struct rr_queue_item_struct rr_queue_item_t;

typedef struct {
  rr_queue_item_t* request_queue_head;
  rr_queue_item_t** request_queue_tail;
  rr_queue_item_t* handler_queue_head;
  rr_queue_item_t** handler_queue_tail;
} rr_queue_t;

void
rr_queue_init (rr_queue_t* rr);

bool
rr_queue_request_empty (const rr_queue_t* rr);

bool
rr_queue_handler_empty (const rr_queue_t* rr);

const rr_queue_item_t*
rr_queue_request_front (const rr_queue_t* rr);

const rr_queue_item_t*
rr_queue_handler_front (const rr_queue_t* rr);

void
rr_queue_push (rr_queue_t* rr,
	       bd_t bd,
	       size_t bd_size,
	       void (*handler) (void*, int, bd_t, size_t),
	       void* data);

void
rr_queue_shift (rr_queue_t* rr);

void
rr_queue_pop (rr_queue_t* rr);

bd_t
rr_queue_item_bd (const rr_queue_item_t* item);

size_t
rr_queue_item_size (const rr_queue_item_t* item);

void*
rr_queue_item_handler (const rr_queue_item_t* item);

void*
rr_queue_item_data (const rr_queue_item_t* item);

#endif /* RR_QUEUE_H */
