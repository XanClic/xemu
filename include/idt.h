#ifndef IDT_H
#define IDT_H

#include <stdint.h>


struct xdt_gate_desc
{
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t rsvd;
    uint8_t type;
    uint16_t offset_hi;
} __attribute__((packed));

#endif
