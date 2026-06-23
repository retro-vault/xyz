        ; fixed24_8_isfinite.s
        ;
        ; All 24.8 fixed values are finite.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_isfinite
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_isfinite

        .area   _CODE

_fixed24_8_isfinite::
        ld      de,#1
        ret
