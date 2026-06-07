        ; atol.s — strtol(nptr, NULL, 10).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module atol
        .optsdcc -mz80 sdcccall(1)
        .globl  _atol
        .globl  _strtol
        .area   _CODE
_atol::
        ld      de,#0
        ld      bc,#10
        push    bc
        call    _strtol
        pop     bc
        ret
