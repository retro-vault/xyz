        ; Report the CPC lower-memory heap below the private C stack.

        .module heap_region
        .optsdcc -mz80 sdcccall(1)
        .globl  _heap_region

CPC_HEAP_LIMIT  .equ    0x9f00

        .area   _HEAP
__cpc_heap_base:

        .area   _CODE
_heap_region::
        ld      hl,#__cpc_heap_base
        ld      de,#CPC_HEAP_LIMIT
        ret
