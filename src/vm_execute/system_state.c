#include <stdint.h>

#include "system_state.h"


uint32_t cr[8] = { 0x00000001, 0, 0, 0, 0x00000000, 0, 0, 0 };

bool int_flag = false;

int iopl = 0;

struct Xdtr gdtr;
struct gdt_desc_cache gdt_desc_cache[SEL_COUNT];
