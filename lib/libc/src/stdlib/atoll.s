        ; atoll.s — strtoll(nptr, NULL, 10).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module atoll
        .optsdcc -mz80 sdcccall(1)
        .globl  _atoll
        .globl  _strtoll
        .area   _CODE
_atoll::
        ld      de,#0
        ld      bc,#10
        push    bc
        call    _strtoll
        pop     bc
        ret
