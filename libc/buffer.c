#include "buffer.h"
#include "syscall_errno.h"
#include <lily/syscall.h>

/* inline bid_t */
/* buffer_create (size_t size) */
/* { */
/*   bid_t bid; */
/*   asm ("int $0x81\n" : "=a"(bid) : "0"(BUFFER_CREATE), "b"(size) :); */
/*   return bid; */
/* } */

/* inline bid_t */
/* buffer_copy (bid_t b, */
/* 	     size_t offset, */
/* 	     size_t length) */
/* { */
/*   bid_t bid; */
/*   asm ("int $0x81\n" : "=a"(bid) : "0"(BUFFER_COPY), "b"(b), "c"(offset), "d"(length) :); */
/*   return bid; */
/* } */

/* inline size_t */
/* buffer_grow (bid_t bid, */
/* 	     size_t size) */
/* { */
/*   size_t off; */
/*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_GROW), "b"(bid), "c"(size) :); */
/*   return off; */
/* } */

/* inline size_t */
/* buffer_append (bid_t dest, */
/* 	       bid_t src, */
/* 	       size_t offset, */
/* 	       size_t length) */
/* { */
/*   size_t off; */
/*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_APPEND), "b"(dest), "c"(src), "d"(offset), "S"(length) :); */
/*   return off; */
/* } */

/* inline int */
/* buffer_assign (bid_t dest, */
/* 	       size_t dest_offset, */
/* 	       bid_t src, */
/* 	       size_t src_offset, */
/* 	       size_t length) */
/* { */
/*   int result; */
/*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_ASSIGN), "b"(dest), "c"(dest_offset), "d"(src), "S"(src_offset), "D"(length) :); */
/*   return result; */
/* } */

void*
buffer_map (bid_t bid)
{
  asm ("int $0x80\n"
       "mov %%ebx, %0\n": "=m"(syscall_errno) : "a"(LILY_SYSCALL_BUFFER_MAP));
}

/* inline int */
/* buffer_unmap (bid_t bid) */
/* { */
/*   int result; */
/*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_UNMAP), "b"(bid) :); */
/*   return result; */
/* } */

/* inline int */
/* buffer_destroy (bid_t bid) */
/* { */
/*   int result; */
/*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_DESTROY), "b"(bid) :); */
/*   return result; */
/* } */

/* inline size_t */
/* buffer_size (bid_t bid) */
/* { */
/*   size_t size; */
/*   asm ("int $0x81\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bid) :); */
/*   return size; */
/* } */
