        ; Complex phase stub for the merged runtime.
        ; Returns 0.0f until a full math helper library is linked.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module cargf
        .area   _CODE
        .globl  cargf

        ; cargf
        ; inputs: xcc helper ABI.
        ; outputs: DE:HL = 0.0f.
        ; clobbers: DE, HL.

cargf:
        ld      hl, #0
        ld      de, #0
        ret
