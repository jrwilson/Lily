#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <lily/syscall.h>
#include <lily/types.h>

#define AUTOMATON_ESUCCESS LILY_SYSCALL_ESUCCESS
#define AUTOMATON_EBADBID LILY_SYSCALL_EBADBID
#define AUTOMATON_EINVAL LILY_SYSCALL_EINVAL

extern int automatonerrno;

aid_t
create (bd_t buffer,
	size_t buffer_size,
	bool retain_privilege);

bid_t
bind (aid_t output_automaton,
      ano_t output_action,
      const void* output_parameter,
      aid_t input_automaton,
      ano_t input_action,
      const void* input_parameter);

#endif /* AUTOMATON_H */
