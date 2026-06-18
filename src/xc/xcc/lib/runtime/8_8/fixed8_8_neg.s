        ; fixed8_8_neg.s
        ;
        ; Signed 8.8 negation. Two's-complement overflow wraps.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_neg
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_neg

        .area   _CODE

        ; inputs:  HL = a
        ; outputs: DE = -a
_fixed8_8_neg::
        xor     a
        sub     a,l
        ld      e,a
        ld      a,#0
        sbc     a,h
        ld      d,a
        ret
