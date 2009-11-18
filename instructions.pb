Macro DPrintN(text)
    PrintN("[0x" + RSet(Hex(*ctx\eip), 8, "0") + "] " + text)
EndMacro
Macro DPrint(text)
    Print("[0x" + RSet(Hex(*ctx\eip), 8, "0") + "] " + text)
EndMacro

Structure reg_arr
    m.s[16]
EndStructure

Global handle_tb_opcode.arr256, name_reg16.reg_arr, name_reg32.reg_arr, name_sreg.reg_arr

Declare lgdt_lidt(*instr_base, *ctx.sigcontext)
Declare ltr_lldt(*instr_base, *ctx.sigcontext)

Procedure init_instructions()
    FillMemory(@handle_tb_opcode, 1024)

    handle_tb_opcode\m[$00] = @ltr_lldt()
    handle_tb_opcode\m[$01] = @lgdt_lidt()

    name_reg16\m[0] = "ax"
    name_reg16\m[1] = "cx"
    name_reg16\m[2] = "dx"
    name_reg16\m[3] = "bx"
    name_reg16\m[4] = "sp"
    name_reg16\m[5] = "bp"
    name_reg16\m[6] = "si"
    name_reg16\m[7] = "di"
    name_reg32\m[0] = "eax"
    name_reg32\m[1] = "ecx"
    name_reg32\m[2] = "edx"
    name_reg32\m[3] = "ebx"
    name_reg32\m[4] = "esp"
    name_reg32\m[5] = "ebp"
    name_reg32\m[6] = "esi"
    name_reg32\m[7] = "edi"
    name_sreg\m[0] = "es"
    name_sreg\m[1] = "???"
    name_sreg\m[2] = "ss"
    name_sreg\m[3] = "ds"
    name_sreg\m[4] = "fs"
    name_sreg\m[5] = "gs"
    name_sreg\m[6] = "???"
    name_sreg\m[7] = "???"
EndProcedure

ProcedureC out_dx_al(*instr_base, *ctx.sigcontext)
    DPrintN("out: " + RSet(Hex(*ctx\eax & $FF), 2, "0") + " -> " + RSet(Hex(*ctx\edx & $FFFF), 4, "0"))
    Select *ctx\edx & $FFFF
        Case $3D0 To $3DC
            ;CGA - wird ignoriert
            ProcedureReturn 1
        Case $3F8 To $3FF, $2F8 To $2FF, $3E8 To $3EF, $2E8 To $2EF
            ;COM - wird ignoriert
            ProcedureReturn 1
    EndSelect
    DPrintN(" -> Unknown port!")
    ProcedureReturn 0
EndProcedure

ProcedureC two_byte_instr(*instr_base, *ctx.sigcontext)
    If Not handle_tb_opcode\m[PeekB(*instr_base + 1) & $FF]
        ProcedureReturn 0
    EndIf
    ret = CallCFunctionFast(handle_tb_opcode\m[PeekB(*instr_base + 1) & $FF], *instr_base, *ctx)
    If ret
        ProcedureReturn ret + 1
    EndIf
    ProcedureReturn 0
EndProcedure

ProcedureC ltr_lldt(*instr_base, *ctx.sigcontext)
    Select PeekB(*instr_base + 2) & $F8
        Case $D8
            DPrintN("ltr " + name_reg16\m[PeekB(*instr_base + 2) & $07])
            If Not *gdt
                DPrintN(" -> unchanged GDT, ltr unallowed.")
                ProcedureReturn 0
            EndIf
            Select PeekB(*instr_base + 2) & $07
                Case 0
                    new_tr = *ctx\eax & $FFFF
                Case 1
                    new_tr = *ctx\ecx & $FFFF
                Case 2
                    new_tr = *ctx\edx & $FFFF
                Case 3
                    new_tr = *ctx\ebx & $FFFF
                Case 4
                    new_tr = *ctx\esp & $FFFF
                Case 5
                    new_tr = *ctx\ebp & $FFFF
                Case 6
                    new_tr = *ctx\esi & $FFFF
                Case 7
                    new_tr = *ctx\edi & $FFFF
            EndSelect
            *ntr.gdt = *gdt + (new_tr & $FFF8)
            If *ntr\access & %10011111 <> %10001001
                DPrint(" -> GPF because of")
                If *ntr\access & %00010000
                    Print(" no_gate")
                EndIf
                If Not *ntr\access & %10000000
                    Print(" na")
                EndIf
                If *ntr\access & $0F <> $09
                    Print(" no_tss")
                EndIf
                PrintN("")
                exception(#EXCEPTION_GPF, *ctx)
            EndIf
            tr = new_tr
            ProcedureReturn 2
    EndSelect

    DPrintN("Unknown command.")

    ProcedureReturn 0
EndProcedure

ProcedureC lgdt_lidt(*instr_base, *ctx.sigcontext)
    Select PeekB(*instr_base + 2) & $F8
        Case $10
            PrintN("lgdt [" + name_reg32\m[PeekB(*instr_base + 2) & $07] + "]")
            Select PeekB(*instr_base + 2) & $07
                Case 0
                    *gdt_dsc.gdt_desc = *ctx\eax
                Case 1
                    *gdt_dsc.gdt_desc = *ctx\ecx
                Case 2
                    *gdt_dsc.gdt_desc = *ctx\edx
                Case 3
                    *gdt_dsc.gdt_desc = *ctx\ebx
                Case 4
                    *gdt_dsc.gdt_desc = *ctx\esp
                Case 5
                    *gdt_dsc.gdt_desc = *ctx\ebp
                Case 6
                    *gdt_dsc.gdt_desc = *ctx\esi
                Case 7
                    *gdt_dsc.gdt_desc = *ctx\edi
            EndSelect
            If *gdt
                FreeMemory(*gdt)
            EndIf
            *gdt = AllocateMemory(*gdt_dsc\limit + 1)
            CopyMemory(*gdt_dsc\base, *gdt, *gdt_dsc\limit + 1)
            DPrintN(" -> " + Str((*gdt_dsc\limit + 1) >> 3) + " entries @0x" + RSet(Hex(*gdt_dsc\base), 8, "0"))
            ProcedureReturn 2
        Case $18
            PrintN("lidt [" + name_reg32\m[PeekB(*instr_base + 2) & $07] + "]")
            Select PeekB(*instr_base + 2) & $07
                Case 0
                    *idt_dsc.idt_desc = *ctx\eax
                Case 1
                    *idt_dsc.idt_desc = *ctx\ecx
                Case 2
                    *idt_dsc.idt_desc = *ctx\edx
                Case 3
                    *idt_dsc.idt_desc = *ctx\ebx
                Case 4
                    *idt_dsc.idt_desc = *ctx\esp
                Case 5
                    *idt_dsc.idt_desc = *ctx\ebp
                Case 6
                    *idt_dsc.idt_desc = *ctx\esi
                Case 7
                    *idt_dsc.idt_desc = *ctx\edi
            EndSelect
            If *idt
                FreeMemory(*idt)
            EndIf
            *idt = AllocateMemory(*idt_dsc\limit + 1)
            CopyMemory(*idt_dsc\base, *idt, *idt_dsc\limit + 1)
            DPrintN(" -> " + Str((*idt_dsc\limit + 1) >> 3) + " entries @0x" + RSet(Hex(*idt_dsc\base), 8, "0"))
            ProcedureReturn 2
    EndSelect

    DPrintN("Unknown command.")

    ProcedureReturn 0
EndProcedure

ProcedureC far_jmp(*instr_base, *ctx.sigcontext)
    offset = PeekL(*instr_base + 1)
    selector = PeekW(*instr_base + 5) & $FFFF

    DPrintN("jmp far 0x" + RSet(Hex(selector), 4, "0") + ":0x" + RSet(Hex(offset), 8, "0"))

    If Not *gdt
        DPrintN(" -> unchanged GDT, jump unallowed.")
        ProcedureReturn 0
    EndIf

    *new_cs.gdt = *gdt + (selector & $FFF8)
    If *new_cs\access & %10011000 <> %10011000 Or ((*new_cs\access & %01100000) >> 5) < cs & $07 Or Not *new_cs\limit_hi & %01000000 Or selector & $07 <> ((*new_cs\access & %01100000) >> 5)
        DPrint(" -> GPF because of")
        If Not *new_cs\access & %00010000
            Print(" no_system")
        EndIf
        If Not *new_cs\access & %00001000
            Print(" not_exec")
        EndIf
        If ((*new_cs\access & %01100000) >> 5) < cs & $07
            Print(" priv_jmp")
        EndIf
        If Not *new_cs\access & %10000000
            Print(" na")
        EndIf
        If Not *new_cs\limit_hi & %01000000
            Print(" not32")
        EndIf
        If selector & $07 <> ((*new_cs\access & %01100000) >> 5)
            Print(" wrong_dpl")
        EndIf
        PrintN("")
        exception(#EXCEPTION_GPF, *ctx)
    EndIf

    cs = selector

    If *new_cs\limit_lo <> $FFFF Or *new_cs\limit_hi & $F <> $F Or Not *new_cs\limit_hi & %10000000
        DPrintN(" -> Warning: limit is below 4 GB. Not checking anything but that could cause problems.")
    EndIf

    If *new_cs\base_lo Or *new_cs\base_mi Or *new_cs\base_hi
        DPrintN(" -> Heavy warning: SEGMENT HAS A BASE, JUMPING TO THE CORRECT ADDRESS BUT IT WON'T BE\n")
        DPrintN("    A SURPRISE IF YOU ENCOUNTER PROBLEMS.\n")
        offset + *new_cs\base_lo + (*new_cs\base_mi << 16) + (*new_cs\base_hi << 24)
    EndIf

    ;Linux-User-CS eintragen
    PokeW(*instr_base + 5, $0073)

    ;FIXME - Wir müssten irgendwann dort wieder den alten Wert hinschreiben
    ProcedureReturn -1
EndProcedure

ProcedureC mov_sreg_reg(*instr_base, *ctx.sigcontext)
    src = PeekB(*instr_base) & $07
    dst = (PeekB(*instr_base) & $38) >> 3

    DPrintN("mov " + name_sreg\m[dst] + "," + name_reg16\m[src])

    Select src
        Case 0
            val = *ctx\eax & $FFFF
        Case 1
            val = *ctx\ecx & $FFFF
        Case 2
            val = *ctx\edx & $FFFF
        Case 3
            val = *ctx\ebx & $FFFF
        Case 4
            val = *ctx\esp & $FFFF
        Case 5
            val = *ctx\ebp & $FFFF
        Case 6
            val = *ctx\esi & $FFFF
        Case 7
            val = *ctx\edi & $FFFF
    EndSelect

    If Not *gdt
        DPrintN(" -> unchanged GDT, mov unallowed.")
        ProcedureReturn 0
    EndIf

    *new_sel.gdt = *gdt + (val & $FFF8)

    If *new_sel\access & %10011000 <> %10010000 Or ((*new_sel\access & %01100000) >> 5) < cs & $07 Or Not *new_sel\limit_hi & %01000000 Or selector & $07 <> ((*new_sel\access & %01100000) >> 5)
        DPrint(" -> GPF because of")
        If Not *new_sel\access & %00010000
            Print(" no_system")
        EndIf
        If *new_sel\access & %00001000
            Print(" exec")
        EndIf
        If ((*new_sel\access & %01100000) >> 5) < cs & $07
            Print(" priv_jmp")
        EndIf
        If Not *new_sel\access & %10000000
            Print(" na")
        EndIf
        If Not *new_sel\limit_hi & %01000000
            Print(" not32")
        EndIf
        If selector & $07 <> ((*new_sel\access & %01100000) >> 5)
            Print(" wrong_dpl")
        EndIf
        PrintN("")
        exception(#EXCEPTION_GPF, *ctx)
    EndIf

    If *new_sel\limit_lo <> $FFFF Or *new_sel\limit_hi & $F <> $F Or Not *new_sel\limit_hi & %10000000
        DPrintN(" -> Warning: limit is below 4 GB. Not checking anything but that could cause problems.")
    EndIf

    If *new_sel\base_lo Or *new_sel\base_mi Or *new_sel\base_hi
        DPrintN(" -> Fatal: Segment has a base. xemu is unable to handle that.\n")
        ProcedureReturn 0
    EndIf

    Select dst
        Case 0
            es = val
        Case 2
            ss = val
        Case 3
            ds = val
        Case 4
            fs = val
        Case 5
            gs = val
        Default
            DPrintN(" -> unknown destination register.")
            ProcedureReturn 0
    EndSelect

    DPrintN(" -> " + name_sreg\m[dst] + " is now 0x" + RSet(Hex(val), 4, "0"))

    ProcedureReturn 2
EndProcedure

ProcedureC sti(*instr_base, *ctx.sigcontext)
    DPrintN("sti")

    ;IOPL überprüfen
    If cs & $07 > ((eflags & $3000) >> 12)
        DPrintN(" -> GPF because CPL > RPL")
        exception(#EXCEPTION_GPF, *ctx)
    EndIf

    eflags | $00000200

    ProcedureReturn 1
EndProcedure
