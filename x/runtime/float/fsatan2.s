        ; Soft-float atan2 stub for the runtime.
        ; Returns 0.0f until a full math helper library is linked.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fsatan2
        .area   _CODE
        .globl  __fsatan2

        ; __fsatan2
        ; inputs: xcc helper ABI.
        ; outputs: DE:HL = 0.0f.
        ; clobbers: DE, HL.

__fsatan2:
        ld      hl, #0
        ld      de, #0
        ret
