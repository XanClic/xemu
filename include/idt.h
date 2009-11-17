#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct idt
{
    uint16_t base_lo;
    uint16_t selector;
    uint8_t  zero;
    unsigned type : 3;
    unsigned is32 : 1;
    unsigned zro : 1;
    unsigned rpl : 2;
    unsigned present : 1;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_desc
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#endif
