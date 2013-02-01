#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "execute.h"
#include "gdt.h"


// TODO: Trap int 0x80 (if the VM guest should execute it)


extern void *vm_comm_area;
extern pid_t vm_pid;


void execute_vm(void)
{
    for (;;)
    {
        int status;
        waitpid(vm_pid, &status, 0);

        assert(WIFSTOPPED(status));

        if (WSTOPSIG(status) == SIGSEGV)
            if (!handle_segfault(vm_pid))
                return;

        ptrace(PTRACE_CONT, vm_pid, NULL, NULL);
    }
}
