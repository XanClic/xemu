#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"

uintptr_t load_elf(void *krnl)
{
    void *base;
    int size;

    printf("Kernel mapped to 0x%08X, interpreting...\n", (unsigned int)krnl);

    Elf32_Ehdr *elf_hdr = krnl;

    if ((elf_hdr->e_ident[0] != 0x7F) ||
        (elf_hdr->e_ident[1] !=  'E') ||
        (elf_hdr->e_ident[2] !=  'L') ||
        (elf_hdr->e_ident[3] !=  'F'))
    {
        fprintf(stderr, "That's no ELF.\n");
        exit(EXIT_FAILURE);
    }

    if ((elf_hdr->e_ident[EI_CLASS] != ELFCLASS32)  ||
        (elf_hdr->e_ident[EI_DATA]  != ELFDATA2LSB) ||
        (elf_hdr->e_machine         != EM_386)      ||
        (elf_hdr->e_type            != ET_EXEC))
    {
        fprintf(stderr, "i386 LSB executable required.\n");
        exit(EXIT_FAILURE);
    }

    printf("Valid format, loading...\n");

    Elf32_Phdr *prg_hdr = (Elf32_Phdr *)((uintptr_t)krnl + elf_hdr->e_phoff);
    for (int i = 0; i < elf_hdr->e_phnum; i++)
    {
        //Laden sollte man das schon können.
        if (prg_hdr[i].p_type != PT_LOAD)
            continue;
        printf("Loading %i bytes (memsize) to 0x%08X\n", (int)prg_hdr[i].p_memsz, (unsigned int)prg_hdr[i].p_vaddr);
        base = (void *)(prg_hdr[i].p_vaddr & 0xFFFFF000);
        size = prg_hdr[i].p_memsz + (prg_hdr[i].p_vaddr & 0x00000FFF);
        printf(" -> padded to %i@0x%08X\n", size, (unsigned int)base);
        memset((void *)prg_hdr[i].p_vaddr, 0, prg_hdr[i].p_memsz);
        if (!prg_hdr[i].p_filesz)
            continue;
        memcpy((void *)prg_hdr[i].p_vaddr, (const void *)((uintptr_t)krnl + prg_hdr[i].p_offset), prg_hdr[i].p_filesz);
    }

    return elf_hdr->e_entry;
}
