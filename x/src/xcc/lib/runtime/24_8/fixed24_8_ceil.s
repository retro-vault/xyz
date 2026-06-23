        ; fixed24_8_ceil.s
        ;
        ; Ceiling via -floor(-x).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_ceil
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_ceil
        .globl  _fixed24_8_neg
        .globl  _fixed24_8_floor

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE:HL = ceil(x)
_fixed24_8_ceil::
        call    _fixed24_8_neg
        call    _fixed24_8_floor
        jp      _fixed24_8_neg
