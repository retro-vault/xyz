        ; abs.s
        ;
        ; libc abs implementation for the xcc Z80 libc.
        ; Returns the absolute value of a 16-bit signed int.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module abs
        .optsdcc -mz80 sdcccall(1)


        .globl  _abs

        .area   _CODE

        ; _abs
        ; inputs:  HL = signed int value
        ; outputs: DE = |value|  (abs(INT_MIN) overflows, as in C)
        ; clobbers: AF
_abs::
        bit     7,h                     ; test sign bit
        jr      nz,.neg
        ld      d,h                     ; non-negative: DE = HL
        ld      e,l
        ret
.neg:
        xor     a
        sub     a,l                     ; DE = 0 - HL  (two's complement)
        ld      e,a
        ld      a,#0
        sbc     a,h
        ld      d,a
        ret
