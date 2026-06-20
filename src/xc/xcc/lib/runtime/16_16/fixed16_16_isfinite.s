        ; fixed16_16_isfinite.s
        ;
        ; All 16.16 fixed values are finite.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_isfinite
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_isfinite

        .area   _CODE

_fixed16_16_isfinite::
        ld      de,#1
        ret
