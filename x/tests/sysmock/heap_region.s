        ;; heap_region.s  (sys backend: sim (in-RAM simulator))
        ;;
        ;; Bare-metal targets have no transient program area to size against, so
        ;; the default heap manages a fixed statically-reserved arena.  Adjust
        ;; SYS_HEAP_SIZE for the target, or create additional heaps explicitly
        ;; with __heap_init_arena over known memory regions.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_region
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_region

SYS_HEAP_SIZE   .equ 8192

        .area   _HEAP
__sys_heap:
        .ds     SYS_HEAP_SIZE
__sys_heap_end:

        .area   _CODE
        ;; void heap_region(void)  ->  HL = base, DE = limit
_heap_region::
        ld      hl,#__sys_heap
        ld      de,#__sys_heap_end
        ret
