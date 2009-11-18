Declare deinit_sdl()

#EXCEPTION_GPF = 13

Procedure exception(num, *ctx.sigcontext)
    *instr = *ctx\eip

    PrintN("Cannot handle exceptions yet.");

    PrintN("Triple Fault. All your base are belong to us.");
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

    End 1
EndProcedure
