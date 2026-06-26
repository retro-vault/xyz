        ;; heap_init_arena.s
        ;;
        ;; Initialise a heap descriptor so it manages the memory region
        ;; [base, limit) as a single free block.  This is the primitive a
        ;; platform (or an application) uses to create a heap — including extra
        ;; heaps for banked blocks or a separate OS heap — without any sbrk.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_init_arena
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_init_arena

BLOCK_SIZE_LO   .equ 0
BLOCK_SIZE_HI   .equ 1
BLOCK_FREE_LO   .equ 2
BLOCK_FREE_HI   .equ 3
BLOCK_NEXT_LO   .equ 4
BLOCK_NEXT_HI   .equ 5
BLOCK_HEAP_LO   .equ 6
BLOCK_HEAP_HI   .equ 7
BLOCK_HDR_SIZE  .equ 8

HEAP_HEAD_LO    .equ 0
HEAP_HEAD_HI    .equ 1
HEAP_BASE_LO    .equ 2
HEAP_BASE_HI    .equ 3
HEAP_LIMIT_LO   .equ 4
HEAP_LIMIT_HI   .equ 5

        .area   _CODE
        ;; void _heap_init_arena(heap, base, limit)
        ;;   HL = heap descriptor, DE = base, limit on stack at 4(ix),5(ix)
_heap_init_arena::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    iy
        push    hl
        pop     iy                      ; IY = heap descriptor
        ld      c,4(ix)
        ld      b,5(ix)                 ; BC = limit
        ld      HEAP_HEAD_LO(iy),e      ; head  = base
        ld      HEAP_HEAD_HI(iy),d
        ld      HEAP_BASE_LO(iy),e      ; base  = base
        ld      HEAP_BASE_HI(iy),d
        ld      HEAP_LIMIT_LO(iy),c     ; limit = limit
        ld      HEAP_LIMIT_HI(iy),b

        ;; payload = (limit - base) - BLOCK_HDR_SIZE
        ld      h,b
        ld      l,c                     ; HL = limit
        or      a
        sbc     hl,de                   ; HL = region size
        ld      bc,#BLOCK_HDR_SIZE
        or      a
        sbc     hl,bc                   ; HL = first block payload size

        push    de
        pop     ix                      ; IX = block @ base
        ld      BLOCK_SIZE_LO(ix),l
        ld      BLOCK_SIZE_HI(ix),h
        ld      BLOCK_FREE_LO(ix),#1
        ld      BLOCK_FREE_HI(ix),#0
        ld      BLOCK_NEXT_LO(ix),#0
        ld      BLOCK_NEXT_HI(ix),#0
        push    iy
        pop     hl                      ; HL = heap descriptor
        ld      BLOCK_HEAP_LO(ix),l
        ld      BLOCK_HEAP_HI(ix),h
        pop     iy
        pop     ix
        ret
