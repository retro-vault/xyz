        ; Soft-float square-root stub for the runtime.
        ; Returns 0.0f until a full math helper library is linked.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fssqrt
        .area   _CODE
        .globl  __fssqrt

        ; __fssqrt
        ; inputs: xcc helper ABI.
        ; outputs: DE:HL = 0.0f.
        ; clobbers: DE, HL.

__fssqrt:
        ld      hl, #0
        ld      de, #0
        ret
