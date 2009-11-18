#REAL_SEGFAULT = 0
#CLI_SEGFAULT = 1

Global expected_segfault = #REAL_SEGFAULT, Dim old_values(19), old_len = 0
Declare segfault_handler(signum.l)

Global handle_opcode.arr256

Procedure init_segfault_handler()
    signal_(#SIGSEGV, @segfault_handler())
    stk.stack_t\ss_sp = mmap_($7F000000, #SIGSTKSZ, #PROT_READ | #PROT_WRITE, #MAP_PRIVATE | #MAP_FIXED | #MAP_ANONYMOUS, -1, 0)
    stk\ss_size = #SIGSTKSZ
    stk\ss_flags = 0
    sigaltstack_(@stk, 0)

    FillMemory(@handle_opcode, 1024)

    init_instructions()

    handle_opcode\m[$0F] = @two_byte_instr()
    handle_opcode\m[$8E] = @mov_sreg_reg()
    handle_opcode\m[$EA] = @far_jmp()
    handle_opcode\m[$EE] = @out_dx_al()
    handle_opcode\m[$FB] = @sti()
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

    If handle_opcode\m[PeekB(*instr) & $FF]
        may_return = CallCFunctionFast(handle_opcode\m[PeekB(*instr) & $FF], *instr, *ctx)
    EndIf

    If may_return
        If may_return > 0
            For i = 0 To may_return
                old_values(i) = PeekB(*instr + i) & $FF
            Next
            old_len = may_return + 1
            expected_segfault = #CLI_SEGFAULT
            For i = 0 To may_return - 1
                PokeB(*instr + i, $90 & $FF) ;NOP
            Next
            PokeB(*instr + may_return, $FA & $FF) ;CLI
            ProcedureReturn 0
        EndIf
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

ProcedureC segfault_handler(signum.l)
    !lea eax,[p.v_signum]
    !add eax,4
    !mov [p_addr],eax
    _segfault_handler(signum, *addr)
EndProcedure
