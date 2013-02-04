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

enum gate_types
{
    GATE_INTR = 0x0e,
    GATE_TRAP = 0x0f
};

#endif
