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
#include "system_state.h"


#define regval(v) ((v) & 0xffffffff)


static const const char *seg_name[] = { "cs", "ds", "es", "fs", "gs", "ss" };


static void unhandled_segfault(pid_t pid, siginfo_t *siginfo, struct user_regs_struct *regs)
{

    fprintf(stderr, "\n=== unhandled segfault ===\n\n");

    switch (siginfo->si_code)
    {
        case SEGV_MAPERR:
        case SEGV_ACCERR:
            fprintf(stderr, "Page fault @0x%08lx\n\n", (uintptr_t)siginfo->si_addr);
            break;
        case SI_KERNEL:
            fprintf(stderr, "General protection fault\n\n");
            break;
        default:
            fprintf(stderr, "Unknown exception code %i\n\n", siginfo->si_code);
            break;
    }

    fprintf(stderr, "eax: 0x%08llx   ebx: 0x%08llx   ecx: 0x%08llx   edx: 0x%08llx\n", regval(regs->rax), regval(regs->rbx), regval(regs->rcx), regval(regs->rdx));
    fprintf(stderr, "esi: 0x%08llx   edi: 0x%08llx   ebp: 0x%08llx   esp: 0x%08llx\n", regval(regs->rsi), regval(regs->rdi), regval(regs->rbp), regval(regs->rsp));

    fprintf(stderr, "eip: 0x%08llx   efl: 0x%08llx    cs: 0x%08llx    ss: 0x%08llx\n", regval(regs->rip), regval(regs->eflags), regval(regs->cs), regval(regs->ss));
    fprintf(stderr, " ds: 0x%08llx    es: 0x%08llx    fs: 0x%08llx    gs: 0x%08llx\n\n", regval(regs->ds), regval(regs->es), regval(regs->fs), regval(regs->gs));

    fprintf(stderr, "cs:eip: %04llx:%08llx\nss:esp: %04llx:%08llx\n\n", regval(regs->cs), regval(regs->rip), regval(regs->ss), regval(regs->rsp));

    fprintf(stderr, "cr0: 0x%08x   cr2: 0x%08x   cr3: 0x%08x   cr4: 0x%08x\n\n", cr[0], cr[2], cr[3], cr[4]);

    fprintf(stderr, "Segment cache:\n");
    for (int i = 0; i < SEL_COUNT; i++)
        fprintf(stderr, "%s:  0x%08x   0x%08x   dpl=%i  %s %c%c %s\n", seg_name[i], gdt_desc_cache[i].base, gdt_desc_cache[i].limit, gdt_desc_cache[i].privilege, gdt_desc_cache[i].present ? "p" : "!p", gdt_desc_cache[i].code ? 'x' : 'r', gdt_desc_cache[i].rw ? (gdt_desc_cache[i].code ? 'r' : 'w') : '-', gdt_desc_cache[i].size ? "32b" : "16b");

    fprintf(stderr, "\nVCPU state: IF=%i IOPL=%i\n\n", int_flag, iopl);


    uint8_t *instr = adr_g2h(regs->rip);

    fprintf(stderr, "Next instructions: %02x %02x %02x %02x %02x %02x\n", instr[0], instr[1], instr[2], instr[3], instr[4], instr[5]);


    FILE *ffp = fopen("/tmp/failed_code.bin", "w");
    if (ffp != NULL)
    {
        fwrite(instr, 1, 10, ffp);
        fclose(ffp);
        system("echo \"Disassembly:       `ndisasm -u /tmp/failed_code.bin | head -n 1 | tail -c +11 | sed -e 's/[[:space:]]\\+/\\n\\t/'`\" | sed -e 's/\\t/                   /' >&2");
        remove("/tmp/failed_code.bin");
    }


    ptrace(PTRACE_KILL, pid, NULL, NULL);
}


bool handle_segfault(pid_t vm_pid)
{
    struct user_regs_struct regs;
    siginfo_t siginfo;

    // One's used, the other's ignored. Who cares which is which.
    ptrace(PTRACE_GETREGS, vm_pid, &regs, &regs);
    ptrace(PTRACE_GETSIGINFO, vm_pid, NULL, &siginfo);


    switch (siginfo.si_code)
    {
        case SEGV_MAPERR:
        case SEGV_ACCERR:
            // TODO: Throw exception
            break;
        case SI_KERNEL:
            if (emulate(&regs))
            {
                ptrace(PTRACE_SETREGS, vm_pid, &regs, &regs);
                return true;
            }
    }


    unhandled_segfault(vm_pid, &siginfo, &regs);


    return false;
}
