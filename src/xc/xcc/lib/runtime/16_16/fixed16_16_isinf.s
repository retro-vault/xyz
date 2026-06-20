        ; fixed16_16_isinf.s
        ;
        ; 16.16 fixed has no infinity encoding.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_isinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_isinf

        .area   _CODE

_fixed16_16_isinf::
        ld      de,#0
        ret
