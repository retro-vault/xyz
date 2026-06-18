        ; fixed16_16_to_int.s
        ;
        ; Convert 16.16 fixed to signed int by arithmetic shift right 16.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_to_int
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_to_int

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE = signed int
_fixed16_16_to_int::
        ex      de,hl
        ret
