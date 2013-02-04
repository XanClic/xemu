#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>


extern uint32_t cr[8];
extern volatile bool int_flag;
extern int iopl;

extern uint16_t tr;

extern struct Xdtr
{
    uint16_t limit;
    uint32_t base;
} gdtr, idtr;


enum selectors
{
    CS, DS, ES, FS, GS, SS, SEL_COUNT
};


extern struct gdt_desc_cache
{
    uint32_t base, limit;
    int privilege;
    bool present, code, rw, size;
} gdt_desc_cache[SEL_COUNT];


extern volatile bool settle_threads;
extern int call_int_vector;

#endif
