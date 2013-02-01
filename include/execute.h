#ifndef _EXECUTE_H
#define _EXECUTE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/user.h>


void execute_vm(void);
bool handle_segfault(pid_t vm_pid);
bool emulate(struct user_regs_struct *regs);

uint32_t vm_execute_syscall(uint32_t sysc_no, int parcount, ...);

#endif
