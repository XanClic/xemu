#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "execute.h"
#include "memory.h"


extern void *vm_comm_area;
extern pid_t vm_pid;


uint32_t vm_execute_syscall(uint32_t sysc_no, int parcount, ...)
{
    struct user_regs_struct regs;

    ptrace(PTRACE_GETREGS, vm_pid, &regs, &regs);


    regs.rip = COMM_GUEST_ADDR + 4092;

    // int 0x80
    ((uint8_t *)vm_comm_area)[4092] = 0xcd;
    ((uint8_t *)vm_comm_area)[4093] = 0x80;
    // cli (aka hypervisor call)
    ((uint8_t *)vm_comm_area)[4094] = 0xfa;


    regs.rax = sysc_no;


    va_list va;
    va_start(va, parcount);

    assert((parcount >= 0) && (parcount < 7));

    if (parcount > 0) regs.rbx = va_arg(va, uint32_t);
    if (parcount > 1) regs.rcx = va_arg(va, uint32_t);
    if (parcount > 2) regs.rdx = va_arg(va, uint32_t);
    if (parcount > 3) regs.rsi = va_arg(va, uint32_t);
    if (parcount > 4) regs.rdi = va_arg(va, uint32_t);
    if (parcount > 5) regs.rbp = va_arg(va, uint32_t);


    ptrace(PTRACE_SETREGS, vm_pid, &regs, &regs);

    ptrace(PTRACE_CONT, vm_pid, NULL, NULL);


    int status;
    waitpid(vm_pid, &status, 0);

    assert(WIFSTOPPED(status));
    assert(WSTOPSIG(status) == SIGSEGV);


    // This process's segfault handler is responsible for saving all the
    // register anyway, so no need to restore their old values here.

    ptrace(PTRACE_GETREGS, vm_pid, &regs, &regs);

    assert(regs.rip == COMM_GUEST_ADDR + 4094);

    return regs.rax;
}
