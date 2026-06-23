        ; fixed24_8_abs.s
        ;
        ; Signed 24.8 absolute value.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_abs
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_abs

        .area   _CODE

        ; inputs:  DE:HL = a (DE low16, HL high16)
        ; outputs: DE:HL = abs(a)
_fixed24_8_abs::
        bit     7,h
        ret     z
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
