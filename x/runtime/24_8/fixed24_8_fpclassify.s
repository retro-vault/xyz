        ; fixed24_8_fpclassify.s
        ;
        ; C fpclassify-style classification for signed 24.8 fixed float mode.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_fpclassify
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_fpclassify

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE = FP_ZERO (2) or FP_NORMAL (4)
_fixed24_8_fpclassify::
        ld      a,h
        or      l
        or      d
        or      e
        ld      de,#2
        ret     z
        ld      de,#4
        ret
