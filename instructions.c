#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exceptions.h"
#include "instructions.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"


#define dprintf(...) { printf("[0x%08X] ", (unsigned int)ctx->eip); printf(__VA_ARGS__); }


static int (*handle_tb_opcode[256])(uint8_t *instr_base, struct sigcontext *ctx) = { NULL };
extern union gdt *gdt;
extern struct idt *idt;
extern uint_fast16_t cs, ds, es, fs, gs, ss, tr;
extern uint32_t eflags;

static const char *name_sreg[8] =
{
    "es",
    "???",
    "ss",
    "ds",
    "fs",
    "gs",
    "???",
    "???"
};

static const char *name_reg32[8] =
{
    "eax",
    "ecx",
    "edx",
    "ebx",
    "esp",
    "ebp",
    "esi",
    "edi"
};

static const char *name_reg16[8] =
{
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di"
};


void init_instructions(void)
{
    handle_tb_opcode[0x00] = &ltr_lldt;
    handle_tb_opcode[0x01] = &lgdt_lidt;
}

int out_dx_al(uint8_t *instr_base, struct sigcontext *ctx)
{
    uint16_t port = ctx->edx & 0xFFFF;
    uint8_t val = ctx->eax & 0xFF;

    dprintf("out: 0x%02X -> 0x%04X\n", val, port);
    if ((port >= 0x3D0) && (port <= 0x3DC))
    {
        //CGA - wird ignoriert
        return 1;
    }
    else if (((port >= 0x3F8) && (port <= 0x3FF)) ||
             ((port >= 0x2F8) && (port <= 0x2FF)) ||
             ((port >= 0x3E8) && (port <= 0x3EF)) ||
             ((port >= 0x2E8) && (port <= 0x2EF)))
    {
        //COM - wird ignoriert
        return 1;
    }
    else if ((port == 0x20) || (port == 0x21) || (port == 0xA0) || (port == 0xA1))
    {
        //Programmable Interrupt Controller
        return pic_outb(port, val);
    }
    dprintf(" -> Unknown port!\n");
    return 0;
}

int two_byte_instr(uint8_t *instr_base, struct sigcontext *ctx)
{
    int ret;

    if (handle_tb_opcode[instr_base[1]] == NULL)
        return 0;
    ret = handle_tb_opcode[instr_base[1]](instr_base, ctx);
    if (ret)
        return ++ret;
    return 0;
}

int ltr_lldt(uint8_t *instr_base, struct sigcontext *ctx)
{
    int new_tr;
    struct gate_desc *ntr;

    switch (instr_base[2] & 0xF8)
    {
        case 0xD8:
            dprintf("ltr %s\n", name_reg16[instr_base[2] & 0x07]);

            if (gdt == NULL)
            {
                dprintf(" -> unchanged GDT, ltr unallowed.\n");
                return 0;
            }

            switch (instr_base[2] & 0x07)
            {
                case 0:
                    new_tr = ctx->eax & 0xFFFF;
                    break;
                case 1:
                    new_tr = ctx->ecx & 0xFFFF;
                    break;
                case 2:
                    new_tr = ctx->edx & 0xFFFF;
                    break;
                case 3:
                    new_tr = ctx->ebx & 0xFFFF;
                    break;
                case 4:
                    new_tr = ctx->esp & 0xFFFF;
                    break;
                case 5:
                    new_tr = ctx->ebp & 0xFFFF;
                    break;
                case 6:
                    new_tr = ctx->esi & 0xFFFF;
                    break;
                case 7:
                    new_tr = ctx->edi & 0xFFFF;
                    break;
            }
            ntr = &gdt[new_tr >> 3].gate;
            if (ntr->system || !ntr->present || (ntr->type != 0x09))
            {
                dprintf(" -> GPF because of");
                if (ntr->system)
                    printf(" no_gate");
                if (!ntr->present)
                    printf(" na");
                if (ntr->type != 0x09)
                    printf(" no_tss");
                printf("\n");
                exception(EXCEPTION_GPF, ctx);
            }

            tr = new_tr;

            return 2;
    }

    dprintf("Unknown command.\n");

    return 0;
}

int lgdt_lidt(uint8_t *instr_base, struct sigcontext *ctx)
{
    struct gdt_desc *gdt_desc = NULL;
    struct idt_desc *idt_desc = NULL;
    int off;

    switch (instr_base[2] & 0xF8)
    {
        case 0x10:
            dprintf("lgdt [%s]\n", name_reg32[instr_base[2] & 0x07]);
            switch (instr_base[2] & 0x07)
            {
                case 0:
                    gdt_desc = (struct gdt_desc *)ctx->eax;
                    break;
                case 1:
                    gdt_desc = (struct gdt_desc *)ctx->ecx;
                    break;
                case 2:
                    gdt_desc = (struct gdt_desc *)ctx->edx;
                    break;
                case 3:
                    gdt_desc = (struct gdt_desc *)ctx->ebx;
                    break;
                case 4:
                    gdt_desc = (struct gdt_desc *)ctx->esp;
                    break;
                case 5:
                    gdt_desc = (struct gdt_desc *)ctx->ebp;
                    break;
                case 6:
                    gdt_desc = (struct gdt_desc *)ctx->esi;
                    break;
                case 7:
                    gdt_desc = (struct gdt_desc *)ctx->edi;
                    break;
            }

            if (gdt_desc == NULL)
                return 0;

            if (gdt != NULL)
                free(gdt);

            gdt = malloc(gdt_desc->limit + 1);
            memcpy(gdt, (void *)gdt_desc->base, gdt_desc->limit + 1);

            dprintf(" -> %i entries @0x%08X\n", (gdt_desc->limit + 1) >> 3, gdt_desc->base);

            return 2;

        case 0x18:
            dprintf("lidt [%s]\n", name_reg32[instr_base[2] & 0x07]);
            switch (instr_base[2] & 0x07)
            {
                case 0:
                    idt_desc = (struct idt_desc *)ctx->eax;
                    break;
                case 1:
                    idt_desc = (struct idt_desc *)ctx->ecx;
                    break;
                case 2:
                    idt_desc = (struct idt_desc *)ctx->edx;
                    break;
                case 3:
                    idt_desc = (struct idt_desc *)ctx->ebx;
                    break;
                case 4:
                    idt_desc = (struct idt_desc *)ctx->esp;
                    break;
                case 5:
                    idt_desc = (struct idt_desc *)ctx->ebp;
                    break;
                case 6:
                    idt_desc = (struct idt_desc *)ctx->esi;
                    break;
                case 7:
                    idt_desc = (struct idt_desc *)ctx->edi;
                    break;
            }

            if (idt_desc == NULL)
                return 0;

            if (idt != NULL)
                free(idt);

            idt = malloc(idt_desc->limit + 1);
            memcpy(idt, (void *)idt_desc->base, idt_desc->limit + 1);

            dprintf(" -> %i entries @0x%08X\n", (idt_desc->limit + 1) >> 3, idt_desc->base);

            return 2;

        case 0x50:
            off = (int8_t)instr_base[3];
            if (off < 0)
            {
                dprintf("lgdt [%s-0x%X]\n", name_reg32[instr_base[2] & 0x07], -off);
            }
            else
            {
                dprintf("lgdt [%s+0x%X]\n", name_reg32[instr_base[2] & 0x07], off);
            }

            switch (instr_base[2] & 0x07)
            {
                case 0:
                    gdt_desc = (struct gdt_desc *)(ctx->eax + off);
                    break;
                case 1:
                    gdt_desc = (struct gdt_desc *)(ctx->ecx + off);
                    break;
                case 2:
                    gdt_desc = (struct gdt_desc *)(ctx->edx + off);
                    break;
                case 3:
                    gdt_desc = (struct gdt_desc *)(ctx->ebx + off);
                    break;
                case 4:
                    gdt_desc = (struct gdt_desc *)(ctx->esp + off);
                    break;
                case 5:
                    gdt_desc = (struct gdt_desc *)(ctx->ebp + off);
                    break;
                case 6:
                    gdt_desc = (struct gdt_desc *)(ctx->esi + off);
                    break;
                case 7:
                    gdt_desc = (struct gdt_desc *)(ctx->edi + off);
                    break;
            }

            if (gdt_desc == NULL)
                return 0;

            if (gdt != NULL)
                free(gdt);

            gdt = malloc(gdt_desc->limit + 1);
            memcpy(gdt, (void *)gdt_desc->base, gdt_desc->limit + 1);

            dprintf(" -> %i entries @0x%08X\n", (gdt_desc->limit + 1) >> 3, gdt_desc->base);

            return 3;

        default:
            dprintf("Unknown command.\n");
            return 0;
    }
}

int far_jmp(uint8_t *instr_base, struct sigcontext *ctx)
{
    uint32_t offset = *((uint32_t *)&instr_base[1]);
    uint16_t selector = *((uint16_t *)&instr_base[5]);
    struct system_desc *new_cs;

    dprintf("jmp far 0x%04X:0x%08X\n", selector, offset);

    //Hu? Das kann der doch nicht machen. Der hat doch keine Ahnung, wie die GDT aussieht.
    if (gdt == NULL)
    {
        dprintf(" -> unchanged GDT, jump unallowed.\n");
        return 0;
    }

    new_cs = &gdt[selector >> 3].system;
    if (!new_cs->system || !new_cs->exec || (new_cs->dpl < (cs & 0x07)) || !new_cs->present || !new_cs->is32 || ((selector & 0x07) != new_cs->dpl))
    {
        dprintf(" -> GPF because of");
        if (!new_cs->system)
            printf(" no_system");
        if (!new_cs->exec)
            printf(" not_exec");
        if (new_cs->dpl < (cs & 0x07))
            printf(" priv_jmp");
        if (!new_cs->present)
            printf(" na");
        if (!new_cs->is32)
            printf(" not32");
        if ((selector & 0x07) != new_cs->dpl)
            printf(" wrong_dpl");
        printf("\n"); 
        exception(EXCEPTION_GPF, ctx);
    }

    cs = selector;

    if ((new_cs->limit_lo != 0xFFFF) || (new_cs->limit_hi != 0xF) || !new_cs->gran)
        dprintf(" -> Warning: limit is below 4 GB. Not checking anything but that could cause problems.\n");

    if (new_cs->base_lo || new_cs->base_hi)
    {
        dprintf(" -> Heavy warning: SEGMENT HAS A BASE, JUMPING TO THE CORRECT ADDRESS BUT IT WON'T BE\n");
        dprintf("    A SURPRISE IF YOU ENCOUNTER PROBLEMS.\n");
        offset += new_cs->base_lo + (new_cs->base_hi << 24);
    }

    //Linux-User-CS eintragen
    instr_base[5] = 0x73;
    instr_base[6] = 0x00;

    //FIXME Wir müssten irgendwann dort wieder den alten Wert hinschreiben
    return -1;
}

int mov_sreg_reg(uint8_t *instr_base, struct sigcontext *ctx)
{
    int src = instr_base[1] & 0x07;
    int dst = (instr_base[1] & 0x38) >> 3;
    int val;
    struct system_desc *new_sel;

    dprintf("mov %s,%s\n", name_sreg[dst], name_reg16[src]);

    switch (src)
    {
        case 0:
            val = ctx->eax & 0xFFFF;
            break;
        case 1:
            val = ctx->ecx & 0xFFFF;
            break;
        case 2:
            val = ctx->edx & 0xFFFF;
            break;
        case 3:
            val = ctx->ebx & 0xFFFF;
            break;
        case 4:
            val = ctx->esp & 0xFFFF;
            break;
        case 5:
            val = ctx->ebp & 0xFFFF;
            break;
        case 6:
            val = ctx->esi & 0xFFFF;
            break;
        case 7:
            val = ctx->edi & 0xFFFF;
            break;
    }

    if (gdt == NULL)
    {
        dprintf(" -> unchanged GDT, mov unallowed.\n");
        return 0;
    }

    new_sel = &gdt[val >> 3].system;
    if (!new_sel->system || new_sel->exec || (new_sel->dpl < (cs & 0x07)) || !new_sel->present || !new_sel->is32 || ((val & 0x07) != new_sel->dpl))
    {
        dprintf(" -> GPF because of");
        if (!new_sel->system)
            printf(" no_system");
        if (new_sel->exec)
            printf(" exec");
        if (new_sel->dpl < (cs & 0x07))
            printf(" priv_jmp");
        if (!new_sel->present)
            printf(" na");
        if (!new_sel->is32)
            printf(" not32");
        if ((val & 0x07) != new_sel->dpl)
            printf(" wrong_dpl");
        printf("\n"); 
        exception(EXCEPTION_GPF, ctx);
    }

    if ((new_sel->limit_lo != 0xFFFF) || (new_sel->limit_hi != 0xF) || !new_sel->gran)
        dprintf(" -> Warning: limit is below 4 GB. Not checking anything but that could cause problems.\n");

    if (new_sel->base_lo || new_sel->base_hi)
    {
        dprintf(" -> Fatal: Segment has a base. xemu is unable to handle that.\n");
        return 0;
    }

    switch (dst)
    {
        case 0:
            es = val;
            break;
        case 2:
            ss = val;
            break;
        case 3:
            ds = val;
            break;
        case 4:
            fs = val;
            break;
        case 5:
            gs = val;
            break;
        default:
            dprintf(" -> unknown destination register.\n");
            return 0;
    }

    dprintf(" -> %s is now 0x%04X\n", name_sreg[dst], val);

    return 2;
}

int sti(uint8_t *instr_base, struct sigcontext *ctx)
{
    dprintf("sti\n");

    //IOPL überprüfen
    if ((cs & 0x07) > ((eflags & 0x3000) >> 12))
    {
        dprintf(" -> GPF because CPL > RPL\n");
        exception(EXCEPTION_GPF, ctx);
    }

    eflags |= 0x00000200;

    return 1;
}
