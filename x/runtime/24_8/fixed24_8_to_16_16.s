        ; fixed24_8_to_16_16.s
        ;
        ; Convert 24.8 fixed to 16.16 fixed by shifting left 8.
        ; Overflow wraps.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_to_16_16
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_to_16_16

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE:HL = fixed16_16
_fixed24_8_to_16_16::
        ld      h,l
        ld      l,d
        ld      d,e
        ld      e,#0
        ret
