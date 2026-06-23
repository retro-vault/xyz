        ; fixed24_8_isnan.s
        ;
        ; 24.8 fixed has no NaN encoding.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_isnan
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_isnan

        .area   _CODE

_fixed24_8_isnan::
        ld      de,#0
        ret
