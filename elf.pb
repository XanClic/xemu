Global get_from_asm

Procedure load_elf(*krnl)
    PrintN("Kernel mapped to 0x" + RSet(Hex(*krnl), 8, "0") + ", interpreting...")

    *elf_hdr.Elf32_Ehdr = *krnl;

    If (*elf_hdr\e_ident[0] <> $7F) Or (*elf_hdr\e_ident[1] <> 'E') Or (*elf_hdr\e_ident[2] <> 'L') Or (*elf_hdr\e_ident[3] <> 'F')
        PrintN("That's no ELF.")
        End 1
    EndIf

    If (*elf_hdr\e_ident[#EI_CLASS] <> #ELFCLASS32) Or (*elf_hdr\e_ident[#EI_DATA] <> #ELFDATA2LSB) Or (*elf_hdr\e_machine <> #EM_386) Or (*elf_hdr\e_type <> #ET_EXEC)
        PrintN("i386 LSB executable required.")
        End 1
    EndIf

    PrintN("Valid format, loading...")

    ;128 MB mappen (außer die ersten 64 kB, die erlaubt Linux uns nicht)
    If mmap_($00010000, $07FF0000, #PROT_READ | #PROT_WRITE | #PROT_EXEC, #MAP_PRIVATE | #MAP_FIXED | #MAP_ANONYMOUS, -1, 0) = #MAP_FAILED
        PrintN("Cannot allocate 128 MB.")
        End 1
    EndIf

    For i = 0 To *elf_hdr\e_phnum - 1
        *prg_hdr.Elf32_Phdr = *krnl + *elf_hdr\e_phoff + i * SizeOf(Elf32_Phdr)
        ;Laden sollte man das schon können.
        If *prg_hdr\p_type <> #PT_LOAD
            Continue
        EndIf
        PrintN("Loading " + Str(*prg_hdr\p_memsz) + " bytes (memsize) to 0x" + RSet(Hex(*prg_hdr\p_vaddr), 8, "0"))
        *base = *prg_hdr\p_vaddr & $FFFFF000
        size = *prg_hdr\p_memsz + *prg_hdr\p_vaddr & $00000FFF
        PrintN(" -> padded to " + Str(size) + "@0x" + RSet(Hex(*base), 8, "0"))
        !mov [v_get_from_asm],memset
        CallCFunctionFast(get_from_asm, *prg_hdr\p_vaddr, 0, *prg_hdr\p_memsz)
        If Not *prg_hdr\p_filesz
            Continue
        EndIf
        CopyMemory(*krnl + *prg_hdr\p_offset, *prg_hdr\p_vaddr, *prg_hdr\p_filesz)
    Next

    ProcedureReturn *elf_hdr\e_entry
EndProcedure
