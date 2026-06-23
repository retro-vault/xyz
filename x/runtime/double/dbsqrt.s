        ; double square-root stub for the runtime
        ; returns 0.0 until a full implementation is provided
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbsqrt
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  __dbsqrt
        .globl  __db_zero

        ; __dbsqrt
        ; inputs:  DE:HL:DE':HL' = double
        ; outputs: DE:HL:DE':HL' = 0.0 (stub)
        ; clobbers: af, de, hl, de', hl'
__dbsqrt:
        jp      __db_zero
