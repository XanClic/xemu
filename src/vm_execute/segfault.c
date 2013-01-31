#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>

#include "execute.h"
#include "memory.h"


#define regval(v) ((v) & 0xffffffff)


static void unhandled_segfault(pid_t pid)
{
    struct user_regs_struct regs;
    siginfo_t siginfo;

    // One's used, the other's ignored. Who cares which is which.
    ptrace(PTRACE_GETREGS, pid, &regs, &regs);
    ptrace(PTRACE_GETSIGINFO, pid, NULL, &siginfo);

    uint32_t cr2 = (uintptr_t)siginfo.si_addr;

    fprintf(stderr, "\n=== unhandled segfault ===\n\n");

    fprintf(stderr, "eax: 0x%08llx   ebx: 0x%08llx   ecx: 0x%08llx   edx: 0x%08llx\n", regval(regs.rax), regval(regs.rbx), regval(regs.rcx), regval(regs.rdx));
    fprintf(stderr, "esi: 0x%08llx   edi: 0x%08llx   ebp: 0x%08llx   esp: 0x%08llx\n", regval(regs.rsi), regval(regs.rdi), regval(regs.rbp), regval(regs.rsp));

    fprintf(stderr, "eip: 0x%08llx   efl: 0x%08llx    cs: 0x%08llx    ss: 0x%08llx\n", regval(regs.rip), regval(regs.eflags), regval(regs.cs), regval(regs.ss));
    fprintf(stderr, " ds: 0x%08llx    es: 0x%08llx    fs: 0x%08llx    gs: 0x%08llx\n\n", regval(regs.ds), regval(regs.es), regval(regs.fs), regval(regs.gs));

    fprintf(stderr, "   cr2:    0x%08x\n", (unsigned)cr2);
    fprintf(stderr, "cs:eip: %04llx:%08llx\nss:esp: %04llx:%08llx\n\n", regval(regs.cs), regval(regs.rip), regval(regs.ss), regval(regs.rsp));


    uint8_t *instr = (uint8_t *)adr_g2h(regs.rip);

    fprintf(stderr, "Next instructions: %02x %02x %02x %02x %02x %02x\n", instr[0], instr[1], instr[2], instr[3], instr[4], instr[5]);


    FILE *ffp = fopen("/tmp/failed_code.bin", "w");
    if (ffp != NULL)
    {
        fwrite(instr, 1, 10, ffp);
        fclose(ffp);
        system("echo \"Disassembly:       `ndisasm -u /tmp/failed_code.bin | head -n 1 | tail -c +11 | sed -e 's/[[:space:]]/\\n\\t/'`\" | sed -e 's/\\t/        /' >&2");
        remove("/tmp/failed_code.bin");
    }


    ptrace(PTRACE_KILL, pid, NULL, NULL);
}


bool handle_segfault(pid_t vm_pid)
{
    unhandled_segfault(vm_pid);

    return false;
}
