        ; fixed8_8_fdim.s
        ;
        ; Positive difference for signed 8.8 fixed values.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_fdim
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_fdim
        .globl  _fixed8_8_fmax
        .globl  _fixed8_8_sub

        .area   _CODE

        ; inputs:  HL = x, DE = y
        ; outputs: DE = x > y ? x - y : 0
_fixed8_8_fdim::
        push    hl
        push    de
        call    _fixed8_8_fmax
        pop     bc
        pop     hl
        ; If max(x,y) != x, result is zero.
        ld      a,d
        cp      h
        jr      nz,.zero
        ld      a,e
        cp      l
        jr      nz,.zero
        ld      d,b
        ld      e,c
        jp      _fixed8_8_sub
.zero:
        ld      de,#0
        ret
