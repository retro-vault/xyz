        ; Report the CPC lower-memory heap below the private C stack.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module heap_region
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_region

CPC_HEAP_LIMIT  .equ    0x9f00

        .area   _HEAP
__cpc_heap_base:

        .area   _CODE

        ; heap_region
        ; outputs: HL = first heap byte, DE = one-past-last heap byte
        ; clobbers: de, hl

_heap_region::
        ld      hl,#__cpc_heap_base
        ld      de,#CPC_HEAP_LIMIT
        ret
