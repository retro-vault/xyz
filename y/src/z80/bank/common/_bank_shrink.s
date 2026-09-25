        ; Shrink one live allocation in a selected logical bank.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _bank_shrink
        .optsdcc -mz80 sdcccall(1)

        .globl  __bank_shrink
        .globl  __bank_map
        .globl  __mem_split
        .globl  _mem_free
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .area   _CODE

        ; inputs: HL = payload, DE = retained bytes, A = logical bank
        ; output: DE = payload or zero. Selected bank remains mapped.
__bank_shrink::
        push    ix
        push    hl
        push    de
        call    _enter_critical_section
        call    __bank_map
        pop     de
        pop     hl
        push    hl
        ld      bc,#-7
        add     hl,bc
        push    hl
        pop     ix
        bit     0,4(ix)
        jr      z,.failed
        call    __mem_split
        jr      c,.kept
        ex      de,hl
        ld      hl,#0xc000
        call    _mem_free
.kept:
        pop     de
.leave:
        pop     ix
        jp      _leave_critical_section
.failed:
        pop     hl
        ld      de,#0
        jr      .leave
