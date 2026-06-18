        ; fixed16_16_to_8_8.s
        ;
        ; Convert 16.16 fixed to 8.8 fixed by arithmetic shift right 8.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_to_8_8
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_to_8_8

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE = fixed8_8
_fixed16_16_to_8_8::
        ld      e,d
        ld      d,l
        ret
