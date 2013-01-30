#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/user.h>


void unhandled_segfault(pid_t pid)
{
    struct user_regs_struct regs;
    siginfo_t siginfo;

    // One's used, the other's ignored. Who cares which is which.
    ptrace(PTRACE_GETREGS, pid, &regs, &regs);
    ptrace(PTRACE_GETSIGINFO, pid, NULL, &siginfo);

    uint32_t cr2 = (uintptr_t)siginfo.si_addr;

    fprintf(stderr, "\n=== unhandled segfault ===\n\n");

    fprintf(stderr, "eax: 0x%08llx   ebx: 0x%08llx   ecx: 0x%08llx   edx: 0x%08llx\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
    fprintf(stderr, "esi: 0x%08llx   edi: 0x%08llx   ebp: 0x%08llx   esp: 0x%08llx\n", regs.rsi, regs.rdi, regs.rbp, regs.rsp);

    fprintf(stderr, "eip: 0x%08llx   efl: 0x%08llx    cs: 0x%08llx    ss: 0x%08llx\n", regs.rip, regs.eflags, regs.cs, regs.ss);
    fprintf(stderr, " ds: 0x%08llx    es: 0x%08llx    fs: 0x%08llx    gs: 0x%08llx\n\n", regs.ds, regs.es, regs.fs, regs.gs);

    fprintf(stderr, "   cr2:    0x%08x\n", (unsigned)cr2);
    fprintf(stderr, "cs:eip: %04llx:%08llx\nss:esp: %04llx:%08llx\n", regs.cs, regs.rip, regs.ss, regs.rsp);

    ptrace(PTRACE_KILL, pid, NULL, NULL);
}


int main(void)
{
    int shmfd = shm_open("/xemu_phys_ram", O_RDWR | O_CREAT, 0777);
    ftruncate(shmfd, 0x08000000);
    dup2(shmfd, 15);

    void *phys_ram = mmap(NULL, 0x08000000, PROT_READ | PROT_WRITE, MAP_SHARED, 15, 0);

    pid_t child;

    if (!(child = fork()))
    {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        raise(SIGSTOP);

        puts("Entering VM");

        execl("./child", "./child", NULL);
    }

    ptrace(PTRACE_ATTACH, child, NULL, NULL);
    //ptrace(PTRACE_SETOPTIONS, child, NULL, (void *)(PTRACE_O_TRACEEXEC | PTRACE_O_EXITKILL));
    ptrace(PTRACE_SETOPTIONS, child, NULL, (void *)PTRACE_O_TRACEEXEC);

    for (;;)
    {
        int status;
        waitpid(child, &status, 0);

        assert(WIFSTOPPED(status));

        if (WSTOPSIG(status) == SIGSEGV)
        {
            unhandled_segfault(child);

            return 0;
        }

        ptrace(PTRACE_CONT, child, NULL, NULL);
    }
}
