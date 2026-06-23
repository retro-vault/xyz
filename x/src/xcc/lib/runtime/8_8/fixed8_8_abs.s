        ; fixed8_8_abs.s
        ;
        ; Signed 8.8 absolute value.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_abs
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_abs

        .area   _CODE

        ; inputs:  HL = a
        ; outputs: DE = abs(a)
_fixed8_8_abs::
        ld      d,h
        ld      e,l
        bit     7,d
        ret     z
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ret
