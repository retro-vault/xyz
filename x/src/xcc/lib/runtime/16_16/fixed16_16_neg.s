        ; fixed16_16_neg.s
        ;
        ; Signed 16.16 negation. Two's-complement overflow wraps.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_neg
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_neg

        .area   _CODE

        ; inputs:  DE:HL = a
        ; outputs: DE:HL = -a
_fixed16_16_neg::
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
