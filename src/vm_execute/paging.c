#include <sys/mman.h>

#include "memory.h"
#include "paging.h"
#include "system_state.h"


void full_paging_update(void)
{
    munmap(adr_g2h(0), 0x100000000ULL);

    if (!(cr[0] & (1 << 31)))
        mmap(adr_g2h(0), MEMSZ, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, 15, 0);

    // Just leave everything else unmapped. Page faults will bring it in.
}
