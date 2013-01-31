#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "execute.h"


void execute_vm(pid_t vm_pid, uintptr_t entry)
{
    for (;;)
    {
        int status;
        waitpid(vm_pid, &status, 0);

        assert(WIFSTOPPED(status));

        if (WSTOPSIG(status) == SIGSEGV)
        {
            if (entry)
            {
                struct user_regs_struct regs;
                ptrace(PTRACE_GETREGS, vm_pid, &regs, &regs);

                regs.rip = entry;

                ptrace(PTRACE_SETREGS, vm_pid, &regs, &regs);

                entry = 0;
            }
            else if (!handle_segfault(vm_pid))
                return;
        }

        ptrace(PTRACE_CONT, vm_pid, NULL, NULL);
    }
}
