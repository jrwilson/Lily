#ifndef IO_H
#define IO_H

#include <lily/syscall.h>
#include <stddef.h>

#define IO_ESUCCESS LILY_SYSCALL_ESUCCESS
#define IO_EPERM LILY_SYSCALL_EPERM
#define IO_EINVAL LILY_SYSCALL_EINVAL
#define IO_ESRCRANGE LILY_SYSCALL_ESRCRANGE
#define IO_ESRCINUSE LILY_SYSCALL_ESRCINUSE
#define IO_EDSTINUSE LILY_SYSCALL_EDSTINUSE

extern int ioerrno;

int
map (const void* destination,
     const void* source,
     size_t size);

#endif /* IO_H */
