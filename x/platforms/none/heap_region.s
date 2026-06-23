        ;; heap_region.s  (sys backend: none — template)
        ;;
        ;; void heap_region(void)
        ;;   returns HL = base, DE = limit (one past the end)
        ;;
        ;; Reports the region the default libc heap manages.  malloc()/free()
        ;; build a heap over it on first use (there is no sbrk).  This template
        ;; reserves a fixed static arena — the simplest portable choice.  On a
        ;; real target you will usually prefer the gap between the end of the
        ;; program image and the stack instead, e.g.:
        ;;
        ;;       ld   hl,#__heap_base        ; base = top of image (_HEAP last)
        ;;       ld   de,#(STACK_TOP - 0x200)
        ;;       ret
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_region
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_region

        ;; TODO: size the heap for your target.
SYS_HEAP_SIZE   .equ 8192

        .area   _HEAP
__heap_base:
        .ds     SYS_HEAP_SIZE
__heap_top:

        .area   _CODE
_heap_region::
        ld      hl,#__heap_base          ; base
        ld      de,#__heap_top      ; limit
        ret
