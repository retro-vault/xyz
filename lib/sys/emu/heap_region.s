        ;; heap_region.s  (sys backend: emu)

        .module heap_region
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_region

        .equ    SYS_HEAP_SIZE,8192

        .area   _HEAP
__heap_base:
        .ds     SYS_HEAP_SIZE
__heap_top:

        .area   _CODE
_heap_region::
        ld      hl,#__heap_base
        ld      de,#__heap_top
        ret
