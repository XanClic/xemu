#ifndef MEMORY_H
#define MEMORY_H

#define adr_g2h(adr) ((uintptr_t)(adr) | 0x100000000UL)
#define adr_h2g(adr) ((uintptr_t)(adr) & 0x0FFFFFFFFUL)

#endif
