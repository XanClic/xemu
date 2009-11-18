Structure gdt_desc
    limit.w
    base.l
EndStructure

Structure gdt
    limit_lo.w
    base_lo.w
    base_mi.b
    access.b
    limit_hi.b
    base_hi.b
EndStructure
