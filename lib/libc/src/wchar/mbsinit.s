        ; mbsinit.s — stateless encoding, always the initial state.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module mbsinit
        .optsdcc -mz80 sdcccall(1)
        .globl  _mbsinit
        .area   _CODE
_mbsinit::
        ld      de,#1
        ret
