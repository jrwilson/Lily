#ifndef IO_H
#define IO_H

#include <lily/types.h>
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

int
reserve_port (unsigned short port);

unsigned char
inb (unsigned short port);

int
outb (unsigned short port,
      unsigned char value);

unsigned short
inw (unsigned short port);

int
outw (unsigned short port,
      unsigned short value);

unsigned long
inl (unsigned short port);

int
outl (unsigned short port,
      unsigned long value);

int
subscribe_irq (int irq,
	       ano_t ano,
	       int param);

#endif /* IO_H */
