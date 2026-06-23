        ; fixed16_16_floor.s
        ;
        ; Floor signed 16.16 by clearing the fractional word.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_floor
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_floor

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE:HL = floor(x)
_fixed16_16_floor::
        ld      de,#0
        ret
