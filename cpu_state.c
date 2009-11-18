#include <stddef.h>
#include <stdint.h>

#include "gdt.h"
#include "idt.h"

union gdt *gdt = NULL;
struct idt *idt = NULL;
uint_fast16_t cs = 0x10, ds = 0x10, es = 0x10, fs = 0x10, gs = 0x10, ss = 0x10, tr = 0x00;
uint32_t eflags = 0x00000002;
