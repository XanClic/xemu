/******************************************************************************
 * Copyright (c) 2009 Hanna Reitz                                             *
 *                                                                            *
 * Permission is hereby granted,  free of charge,  to any  person obtaining a *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction,  including without limitation *
 * the rights to use,  copy, modify, merge, publish,  distribute, sublicense, *
 * and/or sell copies  of the  Software,  and to permit  persons to whom  the *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING  BUT NOT  LIMITED TO THE WARRANTIES OF MERCHANTABILITY, *
 * FITNESS FOR A PARTICULAR  PURPOSE AND  NONINFRINGEMENT.  IN NO EVENT SHALL *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
 * FROM,  OUT OF  OR IN CONNECTION  WITH THE  SOFTWARE  OR THE  USE OR  OTHER *
 * DEALINGS IN THE SOFTWARE.                                                  *
 ******************************************************************************/

#ifndef _ELF32_H
#define _ELF32_H

#include <stdint.h>


/*******************************
 * Nicht unterstütztes Zeug:   *
 *  - 64 bit                   *
 *  - BigEndian                *
 *  - alles außer Executables  *
 *  - alles außer i386         *
 *  - alle unladbaren Segmente *
 *******************************/

#define EI_NIDENT     16
#define EI_CLASS       4
#define EI_DATA        5
#define ELFCLASS32     1
#define ELFCLASS64     2
#define ELFDATA2LSB    1
#define ELFDATA2MSB    2
#define ET_REL         1
#define ET_EXEC        2
#define ET_DYN         3
#define ET_CORE        4
#define EM_386         3    /*       Intel 80386       */
#define EM_PPC        20    /*         PowerPC         */
#define EM_PPC64      21    /*     PowerPC 64-bit      */
#define EM_X86_64     62    /* AMD x86-64 architecture */
#define SH_PROGBITS    1
#define SH_STRTAB      3
#define PT_LOAD        1
#define SHT_SYMTAB     2
#define SHT_STRTAB     3
#define SHF_WRITE      0x01
#define SHF_ALLOC      0x02
#define SHF_EXECINSTR  0x04
#define PF_X           0x01
#define PF_W           0x02
#define PF_R           0x04
#define SH_SYMTAB      2
#define SH_STRTAB      3
#define ELF32_ST_TYPE(_) ((_) & 0xF)
#define STT_NOTYPE     0   /* Symbol type is unspecified */
#define STT_OBJECT     1   /* Symbol is a data object */
#define STT_FUNC       2   /* Symbol is a code object */
#define STT_SECTION    3   /* Symbol associated with a section */
#define STT_FILE       4   /* Symbol's name is file name */
#define STT_COMMON     5   /* Symbol is a common data object */
#define STT_TLS        6   /* Symbol is thread-local data object*/
#define STT_NUM        7   /* Number of defined types.  */
#define STT_LOOS      10    /* Start of OS-specific */
#define STT_HIOS      12    /* End of OS-specific */
#define STT_LOPROC    13    /* Start of processor-specific */
#define STT_HIPROC    15    /* End of processor-specific */

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef  int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

//Mit 64 bit muss ich nicht umgehen können...

typedef struct elf32_hdr
{
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct elf32_phdr
{
    Elf32_Word    p_type;
    Elf32_Off     p_offset;
    Elf32_Addr    p_vaddr;
    Elf32_Addr    p_paddr;
    Elf32_Word    p_filesz;
    Elf32_Word    p_memsz;
    Elf32_Word    p_flags;
    Elf32_Word    p_align;
} __attribute__((packed)) Elf32_Phdr;

typedef struct
{
    Elf32_Word    sh_name;
    Elf32_Word    sh_type;
    Elf32_Word    sh_flags;
    Elf32_Addr    sh_addr;
    Elf32_Off     sh_offset;
    Elf32_Word    sh_size;
    Elf32_Word    sh_link;
    Elf32_Word    sh_info;
    Elf32_Word    sh_addralign;
    Elf32_Word    sh_entsize;
} __attribute__((packed)) Elf32_Shdr;

typedef struct elf32_sym
{
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
} __attribute__((packed)) Elf32_Sym;


uintptr_t load_elf(void *elf_base);

#endif
