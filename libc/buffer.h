#ifndef BUFFER_H
#define BUFFER_H

#include <lily/syscall.h>
#include <lily/types.h>

#define BUFFER_ESUCCESS LILY_SYSCALL_ESUCCESS
#define BUFFER_EBADBID LILY_SYSCALL_EBADBID
#define BUFFER_ENOMEM LILY_SYSCALL_ENOMEM

extern int buffererrno;

bd_t
buffer_create (size_t size);

  /* inline bd_t */
  /* buffer_copy (bd_t b, */
  /* 	       size_t offset, */
  /* 	       size_t length) */
  /* { */
  /*   bd_t bid; */
  /*   asm ("int $0x81\n" : "=a"(bid) : "0"(BUFFER_COPY), "b"(b), "c"(offset), "d"(length) :); */
  /*   return bid; */
  /* } */

  /* inline size_t */
  /* buffer_grow (bd_t bid, */
  /* 	       size_t size) */
  /* { */
  /*   size_t off; */
  /*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_GROW), "b"(bid), "c"(size) :); */
  /*   return off; */
  /* } */

  /* inline size_t */
  /* buffer_append (bd_t dest, */
  /* 		 bd_t src, */
  /* 		 size_t offset, */
  /* 		 size_t length) */
  /* { */
  /*   size_t off; */
  /*   asm ("int $0x81\n" : "=a"(off) : "0"(BUFFER_APPEND), "b"(dest), "c"(src), "d"(offset), "S"(length) :); */
  /*   return off; */
  /* } */

  /* inline int */
  /* buffer_assign (bd_t dest, */
  /* 		 size_t dest_offset, */
  /* 		 bd_t src, */
  /* 		 size_t src_offset, */
  /* 		 size_t length) */
  /* { */
  /*   int result; */
  /*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_ASSIGN), "b"(dest), "c"(dest_offset), "d"(src), "S"(src_offset), "D"(length) :); */
  /*   return result; */
  /* } */

void*
buffer_map (bd_t bid);

int
buffer_unmap (bd_t bid);

  /* inline int */
  /* buffer_destroy (bd_t bid) */
  /* { */
  /*   int result; */
  /*   asm ("int $0x81\n" : "=a"(result) : "0"(BUFFER_DESTROY), "b"(bid) :); */
  /*   return result; */
  /* } */

  /* inline size_t */
  /* buffer_size (bd_t bid) */
  /* { */
  /*   size_t size; */
  /*   asm ("int $0x81\n" : "=a"(size) : "0"(BUFFER_SIZE), "b"(bid) :); */
  /*   return size; */
  /* } */

#endif /* BUFFER_H */
