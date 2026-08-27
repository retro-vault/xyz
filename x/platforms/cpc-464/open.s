        ; CPC 464 file open hook: no disk filesystem is present.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module open
        .optsdcc -mz80 sdcccall(1)

        .globl  _open

        .area   _CODE
_open::
        ld      de,#0xffff
        ret
