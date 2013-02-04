#include <assert.h>
#include <stdbool.h>
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


bool sys_segs_set_up = false;


static void ldt_from_gdt(int i, struct gdt_desc *gdt)
{
    // Just enter segments into the LDT, no gates
    if (!(gdt->type & 0x10))
        return;


    struct user_desc *ldt_desc = vm_comm_area;

    ldt_desc->entry_number    = i;
    ldt_desc->base_addr       = gdt->base_lo | (gdt->base_mi << 16) | (gdt->base_hi << 24);
    ldt_desc->limit           = gdt->limit_lo | ((gdt->limit_hi & 0xf) << 16);
    ldt_desc->seg_32bit       = !!(gdt->flags & 0x40);
    ldt_desc->read_exec_only  =  !(gdt->type & 0x02);
    ldt_desc->limit_in_pages  = !!(gdt->flags & 0x80);
    ldt_desc->seg_not_present =  !(gdt->type & 0x80);
    ldt_desc->useable         = !!(gdt->flags & 0x10);
    ldt_desc->lm              = !!(gdt->flags & 0x20); // awww this is bad

    switch ((gdt->type & 0x0c) >> 2)
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


void gdt_update(void)
{
    int entries = (gdtr.limit + 1) / 8;

    struct gdt_desc *gdt = adr_g2h(gdtr.base);

    for (int i = 1; i < entries; i++)
        ldt_from_gdt(i, &gdt[i]);

    if (!sys_segs_set_up)
    {
        struct gdt_desc sysd = {
            .base_lo = 0x0000,
            .base_mi = 0x00,
            .base_hi = 0x00,
            .limit_lo = 0xffff,
            .limit_hi = 0xcf,
            .type = 0x9a
        };

        ldt_from_gdt(1024, &sysd);

        sysd.type = 0x92;

        ldt_from_gdt(1025, &sysd);

        sys_segs_set_up = true;
    }
}


uint16_t load_seg_reg(int reg, uint16_t value)
{
    assert(!(value & 4));


    // FIXME: Exceptions

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


uint16_t seg_h2g(uint16_t sval)
{
    assert((sval & 7) == 7);


    struct gdt_desc *gdt = (struct gdt_desc *)adr_g2h(gdtr.base) + (sval & 0xfff8);

    return (sval & 0xfff8) | ((gdt->type & 0x60) >> 5);
}
