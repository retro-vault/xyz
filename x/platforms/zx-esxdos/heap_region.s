        .module heap_region
        .optsdcc -mz80 sdcccall(1)
        .globl  _heap_region
        .globl  s__STACK
        .area   _HEAP
__zx_heap_base:
        .area   _CODE
_heap_region::
        ld      hl,#__zx_heap_base
        ld      de,#s__STACK
        ret
        ; The linker script selects the application's stack allowance.
        .area   _STACK
