#EI_NIDENT     = 16
#EI_CLASS      =  4
#EI_DATA       =  5
#ELFCLASS32    =  1
#ELFCLASS64    =  2
#ELFDATA2LSB   =  1
#ELFDATA2MSB   =  2
#ET_REL        =  1
#ET_EXEC       =  2
#ET_DYN        =  3
#ET_CORE       =  4
#EM_386        =  3    ;       Intel 80386
#EM_PPC        = 20    ;         PowerPC
#EM_PPC64      = 21    ;     PowerPC 64-bit
#EM_X86_64     = 62    ; AMD x86-64 architecture
#SH_PROGBITS   =  1
#SH_STRTAB     =  3
#PT_LOAD       =  1
#SHT_SYMTAB    =  2
#SHT_STRTAB    =  3
#SHF_WRITE     =  $01
#SHF_ALLOC     =  $02
#SHF_EXECINSTR =  $04
#PF_X          =  $01
#PF_W          =  $02
#PF_R          =  $04
#SH_SYMTAB     =  2
#SH_STRTAB     =  3
Macro ELF32_ST_TYPE(_)
    ((_) & $F)
EndMacro
#STT_NOTYPE    =  0   ; Symbol type is unspecified
#STT_OBJECT    =  1   ; Symbol is a data object
#STT_FUNC      =  2   ; Symbol is a code object
#STT_SECTION   =  3   ; Symbol associated with a section
#STT_FILE      =  4   ; Symbol's name is file name
#STT_COMMON    =  5   ; Symbol is a common data object
#STT_TLS       =  6   ; Symbol is thread-local data object
#STT_NUM       =  7   ; Number of defined types.
#STT_LOOS      = 10   ; Start of OS-specific
#STT_HIOS      = 12   ; End of OS-specific
#STT_LOPROC    = 13   ; Start of processor-specific
#STT_HIPROC    = 15   ; End of processor-specific


Macro Elf32_Addr
    l
EndMacro
Macro Elf32_Half
    w
EndMacro
Macro Elf32_Off
    l
EndMacro
Macro Elf32_Sword
    l
EndMacro
Macro Elf32_Word
    l
EndMacro

Structure Elf32_Ehdr
    e_ident.b[#EI_NIDENT]
    e_type.Elf32_Half
    e_machine.Elf32_Half
    e_version.Elf32_Word
    e_entry.Elf32_Addr
    e_phoff.Elf32_Off
    e_shoff.Elf32_Off
    e_flags.Elf32_Word
    e_ehsize.Elf32_Half
    e_phentsize.Elf32_Half
    e_phnum.Elf32_Half
    e_shentsize.Elf32_Half
    e_shnum.Elf32_Half
    e_shstrndx.Elf32_Half
EndStructure

Structure Elf32_Phdr
    p_type.Elf32_Word
    p_offset.Elf32_Off
    p_vaddr.Elf32_Addr
    p_paddr.Elf32_Addr
    p_filesz.Elf32_Word
    p_memsz.Elf32_Word
    p_flags.Elf32_Word
    p_align.Elf32_Word
EndStructure

Structure Elf32_Shdr
    sh_name.Elf32_Word
    sh_type.Elf32_Word
    sh_flags.Elf32_Word
    sh_addr.Elf32_Addr
    sh_offset.Elf32_Off
    sh_size.Elf32_Word
    sh_link.Elf32_Word
    sh_info.Elf32_Word
    sh_addralign.Elf32_Word
    sh_entsize.Elf32_Word
EndStructure

Structure Elf32_Sym
    st_name.Elf32_Word
    st_value.Elf32_Addr
    st_size.Elf32_Word
    st_info.b
    st_other.b
    st_shndx.Elf32_Half
EndStructure
