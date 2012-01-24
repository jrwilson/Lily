#ifndef SYSCONF_H
#define SYSCONF_H

#include <lily/syscall.h>

#define SYSCONF_ESUCCESS LILY_SYSCALL_ESUCCESS
#define SYSCONF_EBADBID LILY_SYSCALL_EINVAL

#define PAGESIZE LILY_SYSCALL_SYSCONF_PAGESIZE

extern int sysconferrno;

long
sysconf (int name);

#endif /* SYSCONF_H */
