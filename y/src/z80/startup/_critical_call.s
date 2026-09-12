        ; Compact protected-call entry for routines with register arguments.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _critical_call
        .optsdcc -mz80 sdcccall(1)
        .globl  __critical_call
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .area   _CODE

        ; CALL this immediately before the protected body. Its RET leaves
        ; the section, then returns to the original caller. Adds one stack
        ; word: not suitable for bodies accessing caller stack arguments.
        ; Preserves primary registers, AF, IX/IY; clobbers alternate DE/HL.
__critical_call::
        call    _enter_critical_section
        exx
        pop     hl                      ; protected body's entry
        ld      de,#_leave_critical_section
        push    de                      ; protected body's return
        push    hl
        exx
        ret
