#include <assert.h>
#include <errno.h>
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

extern int call_int_vector;

volatile bool no_irqs_nao = false;


static bool intr(int vector, uint32_t err_code, bool ext)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, vm_pid, &regs, &regs);

    if (!execute_interrupt(&regs, vector, err_code, ext))
        return false;

    ptrace(PTRACE_SETREGS, vm_pid, &regs, &regs);

    return true;
}


void execute_vm(void)
{
    bool initial_trap = true;

    for (;;)
    {
        int status, ret;
        do
            ret = waitpid(vm_pid, &status, 0);
        while ((ret < 0) && (errno = EINTR));

        assert(WIFSTOPPED(status));

        no_irqs_nao = true;

        switch (WSTOPSIG(status))
        {
            case SIGSEGV:
                if (!handle_segfault(vm_pid))
                    return;
                break;

            case SIGUSR1:
                if (call_int_vector > -1)
                {
                    if (!intr(call_int_vector, 0, true))
                        return;

                    call_int_vector = -1;
                }
                break;

            case SIGILL:
                if (!intr(6, 0, false)) // #UD
                    return;
                break;

            case SIGFPE:
                if (!intr(0, 0, false)) // #DE (FIXME: MF/XF?)
                    return;
                break;

            case SIGTRAP:
                if (initial_trap)
                {
                    initial_trap = false;
                    break;
                }

                if (!intr(3, 0, false)) // #BP
                    return;
        }

        no_irqs_nao = false;

        ptrace(PTRACE_CONT, vm_pid, NULL, NULL);
    }
}
