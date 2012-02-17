#ifndef LILY_SYSCALL_H
#define LILY_SYSCALL_H

/*
  Different modes for finishing actions.
  DESTROY means the specified buffer should be destroyed and the output action did execute if applicable.
  RETAIN means the specified buffer should be retained and the output action did execute if applicable.
  NOOP means the output action did not execute.  No action will be taken with the buffer.
  RETAIN and NOOP are equivalent for input and internal actions.
 */
#define LILY_SYSCALL_FINISH_DESTROY 0
#define LILY_SYSCALL_FINISH_RETAIN  1
#define LILY_SYSCALL_FINISH_NOOP    2

/* Unprivileged system calls. */
#define LILY_SYSCALL_FINISH                0x00
#define LILY_SYSCALL_EXIT                  0x01

#define LILY_SYSCALL_CREATE                0x10
#define LILY_SYSCALL_BIND                  0x11
#define LILY_SYSCALL_UNBIND                0x12
#define LILY_SYSCALL_DESTROY               0x13

#define LILY_SYSCALL_SUBSCRIBE_UNBOUND     0x20
#define LILY_SYSCALL_UNSUBSCRIBE_UNBOUND   0x21
#define LILY_SYSCALL_SUBSCRIBE_DESTROYED   0x22
#define LILY_SYSCALL_UNSUBSCRIBE_DESTROYED 0x23

#define LILY_SYSCALL_ADJUST_BREAK          0x30
/*#define LILY_SYSCALL_ADJUST_STACK             0x31*/
  
#define LILY_SYSCALL_BUFFER_CREATE         0x40
#define LILY_SYSCALL_BUFFER_COPY           0x41
#define LILY_SYSCALL_BUFFER_DESTROY        0x42
#define LILY_SYSCALL_BUFFER_SIZE           0x43
#define LILY_SYSCALL_BUFFER_RESIZE         0x44
#define LILY_SYSCALL_BUFFER_ASSIGN         0x45
#define LILY_SYSCALL_BUFFER_APPEND         0x46
#define LILY_SYSCALL_BUFFER_MAP            0x47
#define LILY_SYSCALL_BUFFER_UNMAP          0x48

#define LILY_SYSCALL_SYSCONF               0x50
#define LILY_SYSCALL_SET_REGISTRY          0x51
#define LILY_SYSCALL_GET_REGISTRY          0x52
#define LILY_SYSCALL_DESCRIBE              0x53

/* Privileged system calls. */
#define LILY_SYSCALL_MAP                   0x100
#define LILY_SYSCALL_UNMAP                 0x101

#define LILY_SYSCALL_RESERVE_PORT          0x110
#define LILY_SYSCALL_UNRESERVE_PORT        0x111
#define LILY_SYSCALL_INB                   0x112
#define LILY_SYSCALL_OUTB                  0x113

#define LILY_SYSCALL_SUBSCRIBE_IRQ         0x120
#define LILY_SYSCALL_UNSUBSCRIBE_IRQ       0x121

#define LILY_SYSCALL_SYSLOG                0x130

/* Names for sysconf. */
#define LILY_SYSCALL_SYSCONF_PAGESIZE 0

/* Error codes. */
#define LILY_SYSCALL_ESUCCESS 0
#define LILY_SYSCALL_EPERM 1
#define LILY_SYSCALL_EINVAL 2
#define LILY_SYSCALL_ESRCNA 3
#define LILY_SYSCALL_ESRCINUSE 4
#define LILY_SYSCALL_EDSTINUSE 5
#define LILY_SYSCALL_ENOMEM 7
#define LILY_SYSCALL_EBDDNE 8
#define LILY_SYSCALL_EBDSIZE 9
#define LILY_SYSCALL_EALIGN 10
#define LILY_SYSCALL_ESIZE 11
#define LILY_SYSCALL_EBADANO 12
#define LILY_SYSCALL_EPORTINUSE 13
#define LILY_SYSCALL_EAIDDNE 14
#define LILY_SYSCALL_EMAPPED 15
#define LILY_SYSCALL_EEMPTY 16
#define LILY_SYSCALL_EOAIDDNE 17
#define LILY_SYSCALL_EIAIDDNE 18
#define LILY_SYSCALL_EOBADANO 19
#define LILY_SYSCALL_EIBADANO 20

#endif /* LILY_SYSCALL_H */
