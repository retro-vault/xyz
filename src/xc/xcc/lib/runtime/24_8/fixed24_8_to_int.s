        ; fixed24_8_to_int.s
        ;
        ; Convert 24.8 fixed to signed int by arithmetic shift right 8.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_to_int
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_to_int

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE = signed int
_fixed24_8_to_int::
        ld      e,d
        ld      d,l
        ret
