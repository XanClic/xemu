#include <stddef.h>
#include <stdint.h>

#include "gdt.h"
#include "idt.h"

union gdt *gdt = NULL;
struct idt *idt = NULL;
uint_fast16_t cs = 0x08, ds = 0x08, es = 0x08, fs = 0x08, gs = 0x08, ss = 0x08, tr = 0x00;
uint32_t eflags = 0x00000002;
