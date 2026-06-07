        ; labs.s
        ;
        ; libc labs implementation for the xcc Z80 libc.
        ; Returns the absolute value of a 32-bit signed long.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module labs
        .optsdcc -mz80 sdcccall(1)


        .globl  _labs

        .area   _CODE

        ; _labs
        ; inputs:  DE = low 16 bits, HL = high 16 bits of signed long
        ; outputs: DE = low 16, HL = high 16 of |value|
        ; clobbers: AF
_labs::
        bit     7,h                     ; sign bit of the high word
        ret     z                       ; non-negative: already in DE:HL
        ; negate 32-bit value DE:HL (0 - value), low word first
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
        ret
