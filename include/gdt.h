#ifndef GDT_H
#define GDT_H

#include <stdint.h>

struct gdt_desc
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct system_desc
{
    uint16_t limit_lo;
    unsigned base_lo : 24;
    unsigned accessed : 1;
    unsigned write_read : 1;
    unsigned dir_conf : 1;
    unsigned exec : 1;
    unsigned system : 1;
    unsigned dpl : 2;
    unsigned present : 1;
    unsigned limit_hi : 4;
    unsigned avail : 1;
    unsigned long_mode : 1;
    unsigned is32 : 1;
    unsigned gran : 1;
    uint8_t  base_hi;
} __attribute__((packed));

struct gate_desc
{
    uint16_t limit_lo;
    unsigned base_lo : 24;
    unsigned type : 4;
    unsigned system : 1;
    unsigned dpl : 2;
    unsigned present : 1;
    unsigned limit_hi : 4;
    unsigned avail : 1;
    unsigned long_mode : 1;
    unsigned is32 : 1;
    unsigned gran : 1;
    uint8_t  base_hi;
} __attribute__((packed));

union gdt
{
    struct system_desc system;
    struct gate_desc gate;
} __attribute__((packed));

#endif
