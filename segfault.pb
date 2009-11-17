#REAL_SEGFAULT = 0
#CLI_SEGFAULT = 1

Global expected_segfault = #REAL_SEGFAULT, Dim old_values(19), old_len = 0
Declare segfault_handler(signum.l)
Declare deinit_sdl()

Procedure init_segfault_handler()
    signal_(#SIGSEGV, @segfault_handler())
    stk.stack_t\ss_sp = mmap_($7F000000, #SIGSTKSZ, #PROT_READ | #PROT_WRITE, #MAP_PRIVATE | #MAP_FIXED | #MAP_ANONYMOUS, -1, 0)
    stk\ss_size = #SIGSTKSZ
    stk\ss_flags = 0
    sigaltstack_(@stk, 0)
EndProcedure

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

    deinit_sdl()
EndProcedure

Global *addr

Procedure segfault_handler(signum.l)
    !lea eax,[p.v_signum]
    !add eax,4
    !mov [p_addr],eax
    _segfault_handler(signum, *addr)
    !ret
EndProcedure
