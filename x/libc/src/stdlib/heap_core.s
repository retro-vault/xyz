        ;; heap_core.s
        ;;
        ;; Hand-written allocator core for the xcc Z80 libc.
        ;;
        ;; allocate(heap, n) is the generic primitive: a first-fit search of the
        ;; given heap's block list, splitting the chosen block and returning a
        ;; payload pointer (or 0).  malloc(n) is allocate() on the process-wide
        ;; __libc_default_heap, which is created lazily over the region the
        ;; platform reports through heap_region (no sbrk).
        ;;
        ;; Block header (8 bytes, 2-byte aligned):
        ;;   struct block { size_t size; u16 free; struct block *next; heap *h; }
        ;; The trailing heap back-pointer lets free()/realloc() recover the
        ;; owning heap from a bare payload pointer.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_core
        .optsdcc -mz80 sdcccall(1)

        .globl  _malloc
        .globl  _allocate
        .globl  __libc_align_size
        .globl  __libc_heap_split
        .globl  __libc_heap_head_get
        .globl  __libc_active_heap
        .globl  __libc_default_heap
        .globl  __libc_heap_ready
        .globl  _heap_region
        .globl  _heap_init_arena

BLOCK_FREE_HI   .equ 3
BLOCK_FREE_LO   .equ 2
BLOCK_HDR_SIZE  .equ 8
BLOCK_NEXT_HI   .equ 5
BLOCK_NEXT_LO   .equ 4
BLOCK_SIZE_HI   .equ 1
BLOCK_SIZE_LO   .equ 0

        .area   _CODE

        ;; void *malloc(size_t n)            HL = n  ->  DE = payload | 0
_malloc::
        ld      a,h
        or      l
        jr      nz,malloc_nonzero
        ld      de,#0
        ret
malloc_nonzero:
        push    hl                      ; save size across setup
        call    __libc_heap_setup
        ld      hl,#__libc_default_heap
        ld      (__libc_active_heap),hl
        pop     hl                      ; HL = size
        jr      __alloc_core

        ;; void *allocate(heap_t *h, size_t n)   HL = h, DE = n  ->  DE = ptr | 0
_allocate::
        ld      (__libc_active_heap),hl
        ex      de,hl                   ; HL = size
        ;; fall through to __alloc_core

        ;; __alloc_core: active heap already selected, HL = size -> DE = ptr | 0
__alloc_core:
        push    ix
        ld      a,h
        or      l
        jr      nz,alloc_have_size
        ld      de,#0
        jr      alloc_return

alloc_have_size:
        call    __libc_align_size
        ld      b,h
        ld      c,l                     ; BC = aligned payload size
        call    __libc_heap_head_get    ; HL = first block
alloc_loop:
        ld      a,h
        or      l
        jr      nz,alloc_check_block
        ld      de,#0
        jr      alloc_return

alloc_check_block:
        push    hl
        pop     ix
        ld      a,BLOCK_FREE_LO(ix)
        or      BLOCK_FREE_HI(ix)
        jr      z,alloc_next_block

        ld      a,BLOCK_SIZE_LO(ix)
        sub     c
        ld      a,BLOCK_SIZE_HI(ix)
        sbc     a,b
        jr      c,alloc_next_block

        ld      h,b
        ld      l,c
        call    __libc_heap_split
        xor     a
        ld      BLOCK_FREE_LO(ix),a
        ld      BLOCK_FREE_HI(ix),a
        push    ix
        pop     hl
        ld      de,#BLOCK_HDR_SIZE
        add     hl,de
        ex      de,hl
        jr      alloc_return

alloc_next_block:
        ld      l,BLOCK_NEXT_LO(ix)
        ld      h,BLOCK_NEXT_HI(ix)
        jr      alloc_loop

alloc_return:
        pop     ix
        ret

        ;; Create the default heap over the platform region on first use.
__libc_heap_setup:
        ld      a,(__libc_heap_ready)
        or      a
        ret     nz
        call    _heap_region      ; HL = base, DE = limit
        ld      b,d
        ld      c,e                     ; BC = limit
        ex      de,hl                   ; DE = base
        ld      hl,#__libc_default_heap
        call    _heap_init_arena       ; HL = heap, DE = base, BC = limit
        ld      a,#1
        ld      (__libc_heap_ready),a
        ret
