        ; fixed16_16_isnan.s
        ;
        ; 16.16 fixed has no NaN encoding.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_isnan
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_isnan

        .area   _CODE

_fixed16_16_isnan::
        ld      de,#0
        ret
