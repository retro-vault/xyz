        ; llabs.s
        ;
        ; libc llabs implementation for the xcc Z80 libc.
        ; Returns the absolute value of a 64-bit signed long long.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module llabs
        .optsdcc -mz80 sdcccall(1)


        .globl  _llabs

        .area   _CODE

        ; _llabs
        ; inputs:  DE:HL:DE':HL' = signed long long (DE=lsw .. HL'=msw)
        ; outputs: DE:HL:DE':HL' = |value|
        ; clobbers: AF
_llabs::
        exx
        bit     7,h                     ; sign bit lives in bit63 (H' bit7)
        exx
        ret     z                       ; non-negative: nothing to do
        ; negate the 64-bit value (0 - value), low word first
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ld      a,#0
        sbc     a,l
        ld      l,a
        ld      a,#0
        sbc     a,h
        ld      h,a
        exx
        ld      a,#0
        sbc     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ld      a,#0
        sbc     a,l
        ld      l,a
        ld      a,#0
        sbc     a,h
        ld      h,a
        exx
        ret
