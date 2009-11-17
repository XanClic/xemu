;Ergänzungen der Linux-API (bzw. libc), die PB nicht kennt

Structure sigcontext
    gs.w
    __gsh.w
    fs.w
    __fsh.w
    es.w
    __esh.w
    ds.w
    __dsh.w
    edi.l
    esi.l
    ebp.l
    esp.l
    ebx.l
    edx.l
    ecx.l
    eax.l
    trapno.l
    err.l
    eip.l
    cs.w
    __csh.w
    eflags.l
    esp_at_signal.l
    ss.w
    __ssh.w
    *fpstate
    oldmask.l
    cr2.l
EndStructure

Structure stack_t
    *ss_sp
    ss_size.l
    ss_flags.l
EndStructure

#PROT_READ = $1
#PROT_WRITE = $2
#PROT_EXEC = $4
#MAP_PRIVATE = $2
#MAP_ANONYMOUS = $20
#MAP_FIXED = $10
#MAP_FAILED = -1
#SIGSEGV = 11

;War in meiner libc so drin, wird also OK sein
#SIGSTKSZ = 8192
