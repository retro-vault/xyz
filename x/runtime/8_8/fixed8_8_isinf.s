        ; fixed8_8_isinf.s
        ;
        ; 8.8 fixed has no infinity encoding.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_isinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_isinf

        .area   _CODE

_fixed8_8_isinf::
        ld      de,#0
        ret
