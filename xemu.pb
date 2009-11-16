IncludeFile "elf.pb"
IncludeFile "api.pb"


#REAL_SEGFAULT = 0
#CLI_SEGFAULT = 1

Global expected_segfault = #REAL_SEGFAULT, Dim old_values(19), old_len = 0

Procedure _segfault_handler(signum.l, *ctx.sigcontext)
    may_return = 0

    If expected_segfault = #CLI_SEGFAULT
        *instr = *ctx\eip - (old_len - 1)
        For i = 0 To old_len - 1
            PokeB(*instr + i, old_values(i) & $FF)
        Next
        expected_segfault = #REAL_SEGFAULT
        ProcedureReturn 0
    EndIf

    *instr = *ctx\eip

    Select PeekB(*instr) & $FF
        Case $EE
            PrintN("out: 0x" + RSet(Hex(*ctx\eax & $FF), 2, "0") + " -> 0x" + RSet(Hex(*ctx\edx & $FFFF), 4, "0"))
            ;out dx,al
            If (*ctx\edx & $FFFF >= $3D0 And *ctx\edx & $FFFF <= $3DC)
                ;CGA - wird ignoriert
                may_return = 2
            EndIf
    EndSelect

    If may_return
        For i = 0 To may_return - 1
            old_values(i) = PeekB(*instr + i) & $FF
        Next
        old_len = may_return
        expected_segfault = #CLI_SEGFAULT
        may_return - 1
        For i = 0 To may_return - 1
            PokeB(*instr + i, $90 & $FF) ;NOP
        Next
        PokeB(*instr + may_return, $FA & $FF) ;CLI
        ProcedureReturn 0
    EndIf

    PrintN("Unhandled segfault. All our base are belong to the OS.")
    PrintN("EIP: 0x" + RSet(Hex(*ctx\eip), 8, "0"))
    PrintN("CR2: 0x" + RSet(Hex(*ctx\cr2), 8, "0"))
    PrintN("Instructions: 0x" + RSet(Hex(PeekB(*instr) & $FF), 2, "0") + " 0x" + RSet(Hex(PeekB(*instr + 1) & $FF), 2, "0") + " 0x" + RSet(Hex(PeekB(*instr + 2) & $FF), 2, "0") + " 0x" + RSet(Hex(PeekB(*instr + 3) & $FF), 2, "0") + " 0x" + RSet(Hex(PeekB(*instr + 4) & $FF), 2, "0") + " 0x" + RSet(Hex(PeekB(*instr + 5) & $FF), 2, "0"))

    PrintN("Disassembling...")
    If Not CreateFile(0, "failed_code.bin")
        PrintN("Failed: Cannot write binary data.")
    Else
        WriteData(0, *instr, 10)
        CloseFile(0)
        ndis = RunProgram("ndisasm", "-u failed_code.bin", GetCurrentDirectory(), #PB_Program_Open | #PB_Program_Read)
        RunProgram("head", "-n 1", "", #PB_Program_Open | #PB_Program_Connect | #PB_Program_Wait, ndis)
        DeleteFile("failed_code.bin")
    EndIf

    PrintN("Dump of 0xB8000:")
    *off = $B8000
    Print("+")
    For x = 1 To 80
        Print("-")
    Next
    PrintN("+")
    For y = 0 To 24
        Print("|")
        For x = 0 To 79
            If PeekB(*off) & $FF < $20
                Print(" ")
            Else
                Print(Chr(PeekB(*off) & $FF))
            EndIf
            *off + 2
        Next
        PrintN("|")
    Next
    Print("+")
    For x = 1 To 80
        Print("-")
    Next
    PrintN("+")

    End 1
EndProcedure

Global *addr

Procedure segfault_handler(signum.l)
    !lea eax,[p.v_signum]
    !add eax,4
    !mov [p_addr],eax
    _segfault_handler(signum, *addr)
    !ret
EndProcedure


OpenConsole()

If CountProgramParameters() <> 1
    PrintN("Usage: xemu <kernel>")
    End 1
EndIf

If Not ReadFile(0, ProgramParameter(0))
    PrintN(ProgramParameter(0) + ": No such file or directory.")
    End 1
EndIf

*krnl = mmap_($78000000, Lof(0), #PROT_READ, #MAP_PRIVATE | #MAP_FIXED, fileno_(FileID(0)), 0)
If *krnl = #MAP_FAILED
    PrintN("Cannot load that kernel.")
    End 1
EndIf

CloseFile(0)

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

PrintN("Load done.")

PrintN("In AD 2009 fun was beginning.")

PrintN("Setting up us this program...")

PrintN("(we have no chance to survive, making our segfault handler)")

signal_(#SIGSEGV, @segfault_handler())
stk.stack_t\ss_sp = mmap_($7F000000, #SIGSTKSZ, #PROT_READ | #PROT_WRITE, #MAP_PRIVATE | #MAP_FIXED | #MAP_ANONYMOUS, -1, 0)
stk\ss_size = #SIGSTKSZ
stk\ss_flags = 0
sigaltstack_(@stk, 0)

PrintN("")

;Ja. Ein Inline-ASM-Jump wäre schöner, aber da müssten wir irgendwie die Structure rüberbekommen
;und so ist es einfacher.
CallCFunctionFast(*elf_hdr\e_entry)

;Wer weiß, warum wir hier landen...
End
