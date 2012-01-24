#ifndef BUFFER_H
#define BUFFER_H

#include <lily/syscall.h>
#include <lily/types.h>

#define BUFFER_ESUCCESS LILY_SYSCALL_ESUCCESS
#define BUFFER_EBADBID LILY_SYSCALL_EBADBID
#define BUFFER_ENOMEM LILY_SYSCALL_ENOMEM

extern int buffererrno;

void*
buffer_map (bid_t bid);

#endif /* BUFFER_H */
