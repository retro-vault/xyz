        ; fixed16_16_ceil.s
        ;
        ; Ceiling via -floor(-x).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_ceil
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_ceil
        .globl  _fixed16_16_neg
        .globl  _fixed16_16_floor

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE:HL = ceil(x)
_fixed16_16_ceil::
        call    _fixed16_16_neg
        call    _fixed16_16_floor
        jp      _fixed16_16_neg
