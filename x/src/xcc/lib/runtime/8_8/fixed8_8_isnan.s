        ; fixed8_8_isnan.s
        ;
        ; 8.8 fixed has no NaN encoding.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_isnan
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_isnan

        .area   _CODE

_fixed8_8_isnan::
        ld      de,#0
        ret
