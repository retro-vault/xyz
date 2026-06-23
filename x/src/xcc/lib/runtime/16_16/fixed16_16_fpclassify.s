        ; fixed16_16_fpclassify.s
        ;
        ; C fpclassify-style classification for signed 16.16 fixed float mode.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_fpclassify
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_fpclassify

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE = FP_ZERO (2) or FP_NORMAL (4)
_fixed16_16_fpclassify::
        ld      a,h
        or      l
        or      d
        or      e
        ld      de,#2
        ret     z
        ld      de,#4
        ret
