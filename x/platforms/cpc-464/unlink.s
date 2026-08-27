        ; CPC 464 unlink hook: no disk filesystem is present.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module unlink
        .optsdcc -mz80 sdcccall(1)

        .globl  _unlink

        .area   _CODE
_unlink::
        ld      de,#0xffff
        ret
