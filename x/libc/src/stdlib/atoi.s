        ; atoi.s — (int)strtol(nptr, NULL, 10).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module atoi
        .optsdcc -mz80 sdcccall(1)
        .globl  _atoi
        .globl  _strtol
        .area   _CODE
_atoi::
        ld      de,#0
        ld      bc,#10
        push    bc
        call    _strtol
        pop     bc
        ret                             ; (int) = low 16 already in DE
