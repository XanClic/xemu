IncludeFile "include/elf.pb"
IncludeFile "include/api.pb"

IncludeFile "segfault.pb"
IncludeFile "elf.pb"
IncludeFile "gfx.pb"


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

entry = load_elf($78000000)

PrintN("Load done.")
PrintN("In AD 2009 fun was beginning.")
PrintN("Setting up us this program...")
PrintN("(we have no chance to survive, making our segfault handler)")

init_segfault_handler()

PrintN("")

init_gfx()

;Ja. Ein Inline-ASM-Jump wäre schöner, aber da müssten wir irgendwie die Structure rüberbekommen
;und so ist es einfacher.
CallCFunctionFast(entry)

;Wer weiß, warum wir hier landen...
End
