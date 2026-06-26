        ;; heap_region.s  (sys backend: emu)
        ;;
        ;; Reports a dynamic heap that starts at the end of the linked image
        ;; and grows upward until a fixed ceiling below the runtime stack.
        ;; Keeping the heap out of the flat binary avoids inflating large emu
        ;; test images into the stack/mailbox area.

        .module heap_region
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_region

        .equ    STACK_TOP,0xfa00
        .equ    HEAP_LIMIT,0xf000

        .area   _HEAP
__heap_base:

        .area   _CODE
_heap_region::
        ld      hl,#__heap_base
        ld      de,#HEAP_LIMIT
        ret
