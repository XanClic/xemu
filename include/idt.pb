Structure idt_desc
    limit.w
    base.l
EndStructure

Structure idt
    base_lo.w
    selector.w
    zero.b
    flags.b
    base_hi.w
EndStructure
