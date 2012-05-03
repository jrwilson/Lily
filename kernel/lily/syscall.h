#ifndef LILY_SYSCALL_H
#define LILY_SYSCALL_H

/* Unprivileged system calls. */
#define LILY_SYSCALL_SCHEDULE              0x00
#define LILY_SYSCALL_FINISH                0x01
#define LILY_SYSCALL_EXIT                  0x02

#define LILY_SYSCALL_CREATE                0x10
#define LILY_SYSCALL_BIND                  0x11
#define LILY_SYSCALL_UNBIND                0x12
#define LILY_SYSCALL_DESTROY               0x13
#define LILY_SYSCALL_EXISTS                0x14
#define LILY_SYSCALL_BINDING_COUNT         0x15

#define LILY_SYSCALL_LOG                   0x20

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
#define LILY_SYSCALL_DESCRIBE              0x52
#define LILY_SYSCALL_GETAID                0x53
#define LILY_SYSCALL_GETINITA              0x54
#define LILY_SYSCALL_GETINITB              0x55
#define LILY_SYSCALL_GETMONOTIME           0x56

/* Privileged system calls. */
#define LILY_SYSCALL_MAP                   0x100
#define LILY_SYSCALL_UNMAP                 0x101

#define LILY_SYSCALL_RESERVE_PORT          0x110
#define LILY_SYSCALL_UNRESERVE_PORT        0x111
#define LILY_SYSCALL_INB                   0x112
#define LILY_SYSCALL_OUTB                  0x113
#define LILY_SYSCALL_INW                   0x114
#define LILY_SYSCALL_OUTW                  0x115
#define LILY_SYSCALL_INL                   0x116
#define LILY_SYSCALL_OUTL                  0x117

#define LILY_SYSCALL_SUBSCRIBE_IRQ         0x120
#define LILY_SYSCALL_UNSUBSCRIBE_IRQ       0x121

/* Names for sysconf. */
#define LILY_SYSCALL_SYSCONF_PAGESIZE 0

#endif /* LILY_SYSCALL_H */
