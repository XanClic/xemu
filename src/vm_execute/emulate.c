#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "execute.h"
#include "gdt.h"
#include "memory.h"
#include "paging.h"
#include "pio.h"
#include "system_state.h"


extern pid_t vm_pid;

extern pthread_cond_t irq_update;


enum opcode
{
    mov_sr_r16  = 0x8e,
    retf        = 0xcb,
    iret        = 0xcf,
    out_ii_al   = 0xe6,
    jmp_far     = 0xea,
    out_dx_al   = 0xee,
    hlt         = 0xf4,
    cli         = 0xfa,
    sti         = 0xfb,
    mov_r32_crX = 0x0f20,
    mov_crX_r32 = 0x0f22,
    ltr         = 0x0f00,
    lXdt        = 0x0f01,
    lgdt        = 0x0f0110,
    lidt        = 0x0f0118,
    invlpg      = 0x0f0138
};

enum operands
{
    NONE,
    MOD_RM_R,
    FAR_PTR32,
    PIO_DX,
    PIO_IMM
};


typedef struct
{
    enum opcode op;
    enum operands operand_type;
    int selector_reg;

    union
    {
        struct
        {
            uint32_t *target, *source;
        };

        struct
        {
            uint32_t ofs;
            uint16_t seg;
        } far_ptr;

        struct
        {
            union
            {
                uint16_t *port;
                uint8_t *port8;
            };
            union
            {
                void *valp;
                uint8_t *val8;
                uint16_t *val16;
                uint32_t *val32;
            };
        };
    };
} instruction_t;


static bool decode_opcode(uint8_t **stream, instruction_t *instr)
{
    int op = 0;

    if (**stream == 0x0f)
        op = *((*stream)++);

    op = (op << 8) | *((*stream)++);

    instr->op = op;


    switch (instr->op)
    {
        case mov_r32_crX:
        case mov_crX_r32:
        case lXdt:
        case mov_sr_r16:
        case ltr:
            instr->operand_type = MOD_RM_R;
            instr->selector_reg = DS;
            break;
        case jmp_far:
            instr->operand_type = FAR_PTR32;
            instr->selector_reg = CS;
            break;
        case cli:
        case sti:
        case hlt:
        case retf:
        case iret:
            instr->operand_type = NONE;
            break;
        case out_dx_al:
            instr->operand_type = PIO_DX;
            break;
        case out_ii_al:
            instr->operand_type = PIO_IMM;
            break;
        default:
            return false;
    }


    return true;
}


size_t modrm_r32[8] = {
    offsetof(struct user_regs_struct, rax),
    offsetof(struct user_regs_struct, rcx),
    offsetof(struct user_regs_struct, rdx),
    offsetof(struct user_regs_struct, rbx),
    offsetof(struct user_regs_struct, rsp),
    offsetof(struct user_regs_struct, rbp),
    offsetof(struct user_regs_struct, rsi),
    offsetof(struct user_regs_struct, rdi)
};

int sreg_num[8] = {
     2, // es
     0, // cs
     5, // ss
     1, // ds
     3, // fs
     4, // gs
    -1, // --
    -1  // --
};

size_t sreg_ofs[6] = {
    offsetof(struct user_regs_struct, cs),
    offsetof(struct user_regs_struct, ds),
    offsetof(struct user_regs_struct, es),
    offsetof(struct user_regs_struct, fs),
    offsetof(struct user_regs_struct, gs),
    offsetof(struct user_regs_struct, ss)
};


// TODO: Segment limit checks


static bool decode_operands(uint8_t **stream, instruction_t *instr, struct user_regs_struct *regs)
{
    switch (instr->operand_type)
    {
        case NONE:
            return true;
        case MOD_RM_R:
        {
            int mod_rm_r = *((*stream)++);

            int rm = mod_rm_r & 0x07;
            int r = (mod_rm_r & 0x38) >> 3;

            uint32_t *rmptr = (uint32_t *)((uintptr_t)regs + modrm_r32[rm]);

            switch ((mod_rm_r >> 6) & 3)
            {
                case 0:
                    if (rm == 4) // SIB
                        return false;
                    if (rm == 5) // disp
                    {
                        rmptr = adr_g2h(*(uint32_t *)*stream);
                        *stream += 4;
                    }
                    else
                        rmptr = adr_g2h(*rmptr + gdt_desc_cache[instr->selector_reg].base);
                    break;
                case 1:
                    if (rm == 4) // SIB
                        return false;
                    rmptr = adr_g2h(*rmptr + (int8_t)*((*stream)++) + gdt_desc_cache[instr->selector_reg].base);
                    break;
                case 2:
                {
                    if (rm == 4) // SIB
                        return false;
                    uint32_t disp = *(uint32_t *)*stream;
                    *stream += 4;
                    rmptr = adr_g2h(*rmptr + disp + gdt_desc_cache[instr->selector_reg].base);
                }
            }

            switch ((int)instr->op)
            {
                case mov_r32_crX:
                    // TODO: #UD
                    instr->target = rmptr;
                    instr->source = &cr[r];
                    return true;
                case mov_crX_r32:
                    // TODO: #UD
                    instr->target = &cr[r];
                    instr->source = rmptr;
                    return true;
                case lXdt:
                    if (r == 2)
                        instr->op = lgdt;
                    else if (r == 3)
                        instr->op = lidt;
                    else if (r == 7)
                        instr->op = invlpg;
                    else
                        return false;
                case ltr:
                    instr->source = rmptr;
                    return true;
                case mov_sr_r16:
                    instr->target = (uint32_t *)(uintptr_t)sreg_num[r];
                    instr->source = rmptr;
                    return true;
            }

            return false;
        }
        case FAR_PTR32:
            instr->far_ptr.ofs = *(uint32_t *)*stream;
            *stream += 4;
            instr->far_ptr.seg = *(uint16_t *)*stream;
            *stream += 2;
            return true;
        case PIO_DX:
            instr->port = (uint16_t *)&regs->rdx;
            instr->valp = &regs->rax;
            return true;
        case PIO_IMM:
            instr->port8 = (*stream)++;
            instr->valp  = &regs->rax;
            return true;
    }

    return false;
}


static bool execute(instruction_t *instr, struct user_regs_struct *regs)
{
    switch ((int)instr->op)
    {
        case mov_r32_crX:
            *instr->target = *instr->source;
            return true;
        case mov_crX_r32:
            *instr->target = *instr->source;
            update_cr(instr->target - &cr[0]);
            return true;
        case lgdt:
            gdtr.limit = *(uint16_t *)instr->source;
            gdtr.base  = *(uint32_t *)((uint16_t *)instr->source + 1);
            gdt_update();
            return true;
        case lidt:
            idtr.limit = *(uint16_t *)instr->source;
            idtr.base  = *(uint32_t *)((uint16_t *)instr->source + 1);
            return true;
        case ltr:
            tr = *(uint16_t *)instr->source;
            return true;
        case invlpg:
            invalidate_page((uintptr_t)instr->source);
            return true;
        case jmp_far:
            regs->cs  = load_seg_reg(CS, instr->far_ptr.seg);
            regs->rip = instr->far_ptr.ofs;
            return true;
        case retf:
        {
            uint32_t *stack = adr_g2h(regs->rsp + gdt_desc_cache[SS].base);
            regs->rip = *(stack++);
            regs->cs  = load_seg_reg(CS, *(stack++) & 0xffff);
            regs->rsp = (uintptr_t)adr_h2g(stack);
            return true;
        }
        case iret:
        {
            uint32_t *stack = adr_g2h(regs->rsp + gdt_desc_cache[SS].base);
            int old_priv = gdt_desc_cache[CS].privilege;

            regs->rip = *(stack++);
            regs->cs  = load_seg_reg(CS, *(stack++) & 0xffff);
            regs->eflags = *(stack++);
            iopl = (regs->eflags >> 12) & 3;
            int_flag = regs->eflags & (1 << 9);

            if (old_priv == gdt_desc_cache[CS].privilege)
                regs->rsp = (uintptr_t)adr_h2g(stack);
            else
            {
                regs->rsp = *(stack++);
                regs->ss  = load_seg_reg(SS, *(stack++) & 0xffff);
            }
            return true;
        }
        case mov_sr_r16:
            assert((int)(uintptr_t)instr->target >= 0); // TODO: #UD
            *(uint16_t *)((uintptr_t)regs + sreg_ofs[(uintptr_t)instr->target]) = load_seg_reg((uintptr_t)instr->target, *instr->source & 0xffff);
            return true;
        case cli:
            int_flag = false;
            return true;
        case sti:
            int_flag = true;
            pthread_cond_broadcast(&irq_update);
            return true;
        case hlt:
            if (!int_flag)
            {
                printf("HLT called while IF=0. Killing the VM and halting.\n");
                ptrace(PTRACE_KILL, vm_pid, NULL, NULL);
                for (;;)
                    sleep(1);
            }
            *(uint8_t *)adr_g2h(regs->rip + gdt_desc_cache[CS].base) = 0x90; // nop and FIXME
            return true;
        case out_dx_al:
            out8(*instr->port, *instr->val8);
            return true;
        case out_ii_al:
            out8(*instr->port8, *instr->val8);
            return true;
    }

    return false;
}


bool emulate(struct user_regs_struct *regs)
{
    uint8_t *instr_stream = adr_g2h(regs->rip + gdt_desc_cache[CS].base);

    instruction_t instr = { 0 };
    if (!decode_opcode(&instr_stream, &instr))
        return false;

    if (!decode_operands(&instr_stream, &instr, regs))
        return false;

    if (!execute(&instr, regs))
        return false;


    if ((instr.op != retf) && (instr.op != iret))
        regs->rip = (uintptr_t)adr_h2g(instr_stream) - gdt_desc_cache[CS].base;


    return true;
}
