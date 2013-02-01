#include <assert.h>
#include <stdio.h>
#include <asm/ldt.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>

#include "execute.h"
#include "gdt.h"
#include "memory.h"
#include "system_state.h"


extern void *vm_comm_area;


void gdt_update(void)
{
    int entries = (gdtr.limit + 1) / 8;

    struct gdt_desc *gdt = (struct gdt_desc *)adr_g2h(gdtr.base);

    for (int i = 1; i < entries; i++)
    {
        // Just enter segments into the LDT, no gates
        if (!(gdt[i].type & 0x10))
            continue;


        struct user_desc *ldt_desc = vm_comm_area;

        ldt_desc->entry_number    = i;
        ldt_desc->base_addr       = gdt[i].base_lo | (gdt[i].base_mi << 16) | (gdt[i].base_hi << 24);
        ldt_desc->limit           = gdt[i].limit_lo | ((gdt[i].limit_hi & 0xf) << 16);
        ldt_desc->seg_32bit       = !!(gdt[i].flags & 0x40);
        ldt_desc->read_exec_only  =  !(gdt[i].type & 0x02);
        ldt_desc->limit_in_pages  = !!(gdt[i].flags & 0x80);
        ldt_desc->seg_not_present =  !(gdt[i].type & 0x80);
        ldt_desc->useable         = !!(gdt[i].flags & 0x10);
        ldt_desc->lm              = !!(gdt[i].flags & 0x20); // awww this is bad

        switch ((gdt[i].type & 0x0c) >> 2)
        {
            case 0:
                ldt_desc->contents = MODIFY_LDT_CONTENTS_DATA;
                break;
            case 1:
                ldt_desc->contents = MODIFY_LDT_CONTENTS_STACK;
                break;
            case 2:
            case 3:
                ldt_desc->contents = MODIFY_LDT_CONTENTS_CODE;
        }


        printf("modify_ldt(%i) == %i\n", i, (int)vm_execute_syscall(123, 3, 1, COMM_GUEST_ADDR, sizeof(*ldt_desc)));
    }
}


uint16_t load_seg_reg(int reg, uint16_t value)
{
    assert(!(value & 4));


    struct gdt_desc *gdt = (struct gdt_desc *)adr_g2h(gdtr.base) + (value >> 3);

    gdt_desc_cache[reg].base  = gdt->base_lo | (gdt->base_mi << 16) | (gdt->base_hi << 24);
    gdt_desc_cache[reg].limit = gdt->limit_lo | ((gdt->limit_hi & 0xf) << 16);

    if (gdt->flags & 0x80)
        gdt_desc_cache[reg].limit = (gdt_desc_cache[reg].limit << 12) | 0xfff;

    gdt_desc_cache[reg].privilege =   (gdt->type & 0x60) >> 5;
    gdt_desc_cache[reg].present   = !!(gdt->type & 0x80);
    gdt_desc_cache[reg].code      = !!(gdt->type & 0x08);
    gdt_desc_cache[reg].rw        = !!(gdt->type & 0x02);
    gdt_desc_cache[reg].size      = !!(gdt->flags & 0x40);


    return value | 7; // LDT and user
}
