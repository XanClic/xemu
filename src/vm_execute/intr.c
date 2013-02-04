#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>

#include "execute.h"
#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "system_state.h"


extern pid_t vm_pid;

static int previous_interrupt = -1;


extern void unhandled_segfault(pid_t pid, siginfo_t *siginfo, struct user_regs_struct *regs);


bool execute_interrupt(struct user_regs_struct *regs, int vector, uint32_t errcode, bool ext)
{
    if ((previous_interrupt >= 0) && (previous_interrupt < 20))
    {
        if (previous_interrupt == 8) // #DF
        {
            fprintf(stderr, "Triple fault, exiting.\n");

            siginfo_t siginfo;
            ptrace(PTRACE_GETSIGINFO, vm_pid, NULL, &siginfo);

            unhandled_segfault(vm_pid, &siginfo, regs);

            return false;
        }

        vector = 8;
    }


    struct xdt_gate_desc *gate = (struct xdt_gate_desc *)adr_g2h(idtr.base) + vector;

    if ((uintptr_t)gate - (uintptr_t)adr_g2h(idtr.base) >= idtr.limit)
    {
        previous_interrupt = vector;
        return execute_interrupt(regs, 13, (vector << 3) | 2 | ext, false); // #GP
    }


    if (!(gate->type & (1 << 7)))
    {
        previous_interrupt = vector;
        return execute_interrupt(regs, 13, (vector << 3) | 2 | ext, false);
    }

    if (((gate->type & 0x1f) != GATE_INTR) && ((gate->type & 0x1f) != GATE_TRAP))
    {
        fprintf(stderr, "Unknown IDT descriptor type 0x%02x.\n", gate->type & 0x1f);
        previous_interrupt = vector;
        return execute_interrupt(regs, 13, (vector << 3) | 2 | ext, false);
    }


    // FIXME: Limit check
    uint32_t *stack = (uint32_t *)((uintptr_t)adr_g2h(regs->rsp) + gdt_desc_cache[SS].base);

    if (gdt_desc_cache[CS].privilege != (gate->selector & 3))
    {
        fprintf(stderr, "Privilege change on interrupt, need TR support.\n");

        siginfo_t siginfo;
        ptrace(PTRACE_GETSIGINFO, vm_pid, NULL, &siginfo);

        unhandled_segfault(vm_pid, &siginfo, regs);

        /*
        *(--stack) = regs->ss;
        *(--stack) = regs->rsp;
        */
    }

    *(--stack) = regs->eflags;
    *(--stack) = regs->cs;
    *(--stack) = regs->rip;


    if ((vector == 8) || ((vector >= 10) && (vector <= 14)) || (vector == 17))
        *(--stack) = errcode;


    regs->rsp = (uintptr_t)adr_h2g(stack);

    regs->cs  = load_seg_reg(CS, gate->selector);
    regs->rip = gate->offset_lo | (gate->offset_hi << 16);


    if ((gate->type & 0x1f) == GATE_INTR)
        int_flag = false;


    previous_interrupt = -1;

    return true;
}
