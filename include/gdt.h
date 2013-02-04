#ifndef GDT_H
#define GDT_H

#include <stdint.h>


struct gdt_desc
{
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mi;
    uint8_t type;
    union
    {
        uint8_t flags;
        uint8_t limit_hi;
    };
    uint8_t base_hi;
} __attribute__((packed));


void gdt_update(void);

uint16_t load_seg_reg(int reg, uint16_t value);
uint16_t seg_h2g(uint16_t sval);

#endif
