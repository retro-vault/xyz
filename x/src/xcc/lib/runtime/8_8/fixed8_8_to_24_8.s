        ; fixed8_8_to_24_8.s
        ;
        ; Convert 8.8 fixed to 24.8 fixed.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_to_24_8
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_to_24_8

        .area   _CODE

        ; inputs:  HL = fixed8_8
        ; outputs: DE:HL = fixed24_8
_fixed8_8_to_24_8::
        ld      d,h
        ld      e,l
        ld      hl,#0
        bit     7,d
        ret     z
        dec     hl
        ret
