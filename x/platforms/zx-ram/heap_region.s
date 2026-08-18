        .module heap_region
        .optsdcc -mz80 sdcccall(1)
        .globl  _heap_region
ZX_HEAP_LIMIT  .equ    0xf000
        .area   _HEAP
__zx_heap_base:
        .area   _CODE
_heap_region::
        ld      hl,#__zx_heap_base
        ld      de,#ZX_HEAP_LIMIT
        ret
