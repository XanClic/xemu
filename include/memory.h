#ifndef MEMORY_H
#define MEMORY_H

// guest (virtual) to host
#define adr_g2h(adr) ((void *)((uintptr_t)(adr) | 0x100000000UL))
// host to guest (virtual)
#define adr_h2g(adr) ((void *)((uintptr_t)(adr) & 0x0ffffffffUL))

// physical to host
#define adr_p2h(adr) ((void *)((uintptr_t)(adr) | 0x200000000UL))

#define MEMSZ 0x08000000

#define COMM_GUEST_ADDR 0xffffd000UL

#endif
