        ; CPC 464 rename hook: no disk filesystem is present.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module rename
        .optsdcc -mz80 sdcccall(1)

        .globl  _rename

        .area   _CODE
_rename::
        ld      de,#0xffff
        ret
