        ; fixed8_8_fpclassify.s
        ;
        ; C fpclassify-style classification for signed 8.8 fixed float mode.
        ; Fixed-point has only zero and normal finite values.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_fpclassify
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_fpclassify

        .area   _CODE

        ; inputs:  HL = fixed8_8
        ; outputs: DE = FP_ZERO (2) or FP_NORMAL (4)
_fixed8_8_fpclassify::
        ld      a,h
        or      l
        ld      de,#2
        ret     z
        ld      de,#4
        ret
