#include <stdint.h>
#include <string.h>

#include "elf.h"
#include "memory.h"


uintptr_t load_elf(void *elf_base)
{
    Elf32_Ehdr *elf_hdr = elf_base;


    if ((elf_hdr->e_ident[0] != 0x7F) ||
        (elf_hdr->e_ident[1] !=  'E') ||
        (elf_hdr->e_ident[2] !=  'L') ||
        (elf_hdr->e_ident[3] !=  'F'))
    {
        return 0;
    }

    if ((elf_hdr->e_ident[EI_CLASS] != ELFCLASS32)  ||
        (elf_hdr->e_ident[EI_DATA]  != ELFDATA2LSB) ||
        (elf_hdr->e_machine         != EM_386)      ||
        (elf_hdr->e_type            != ET_EXEC))
    {
        return 0;
    }


    Elf32_Phdr *prg_hdr = (Elf32_Phdr *)((uintptr_t)elf_base + elf_hdr->e_phoff);

    for (int i = 0; i < elf_hdr->e_phnum; i++)
    {
        if (prg_hdr[i].p_type != PT_LOAD)
            continue;

        memset((void *)adr_g2h(prg_hdr[i].p_paddr), 0, prg_hdr[i].p_memsz);

        if (!prg_hdr[i].p_filesz)
            continue;

        memcpy((void *)adr_g2h(prg_hdr[i].p_paddr), (const void *)((uintptr_t)elf_base + prg_hdr[i].p_offset), prg_hdr[i].p_filesz);
   }


   return elf_hdr->e_entry;
}
