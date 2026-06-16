        ;; free.s
        ;;
        ;; deallocate(heap, ptr) returns a block to a specific heap; free(ptr)
        ;; recovers the owning heap from the block's back-pointer and does the
        ;; same.  Both mark the block free and eagerly coalesce neighbours on
        ;; the active heap's list.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module free
        .optsdcc -mz80 sdcccall(1)

        .globl  _free
        .globl  _deallocate
        .globl  __libc_active_heap
        .globl  __libc_heap_head_get
        .globl  __libc_heap_unwrap_user
        .globl  __libc_ptr_to_block

BLOCK_FREE_HI   .equ 3
BLOCK_FREE_LO   .equ 2
BLOCK_HDR_SIZE  .equ 8
BLOCK_HEAP_HI   .equ 7
BLOCK_HEAP_LO   .equ 6
BLOCK_NEXT_HI   .equ 5
BLOCK_NEXT_LO   .equ 4
BLOCK_SIZE_HI   .equ 1
BLOCK_SIZE_LO   .equ 0

        .area   _CODE
__libc_heap_coalesce:
        call    __libc_heap_head_get
heap_coalesce_loop:
        ld      a,h
        or      l
        ret     z
        push    hl
        pop     ix
        ld      e,BLOCK_NEXT_LO(ix)
        ld      d,BLOCK_NEXT_HI(ix)
        ld      a,d
        or      e
        ret     z
        push    de
        pop     iy

        ld      a,BLOCK_FREE_LO(ix)
        or      BLOCK_FREE_HI(ix)
        jr      z,heap_coalesce_advance
        ld      a,BLOCK_FREE_LO(iy)
        or      BLOCK_FREE_HI(iy)
        jr      z,heap_coalesce_advance

        push    ix
        pop     hl
        ld      de,#BLOCK_HDR_SIZE
        add     hl,de
        ld      e,BLOCK_SIZE_LO(ix)
        ld      d,BLOCK_SIZE_HI(ix)
        add     hl,de
        push    iy
        pop     de
        ld      a,l
        cp      e
        jr      nz,heap_coalesce_advance
        ld      a,h
        cp      d
        jr      nz,heap_coalesce_advance

        ld      l,BLOCK_SIZE_LO(ix)
        ld      h,BLOCK_SIZE_HI(ix)
        ld      de,#BLOCK_HDR_SIZE
        add     hl,de
        ld      e,BLOCK_SIZE_LO(iy)
        ld      d,BLOCK_SIZE_HI(iy)
        add     hl,de
        ld      BLOCK_SIZE_LO(ix),l
        ld      BLOCK_SIZE_HI(ix),h
        ld      e,BLOCK_NEXT_LO(iy)
        ld      d,BLOCK_NEXT_HI(iy)
        ld      BLOCK_NEXT_LO(ix),e
        ld      BLOCK_NEXT_HI(ix),d
        push    ix
        pop     hl                      ; re-check the enlarged block
        jr      heap_coalesce_loop

heap_coalesce_advance:
        push    iy
        pop     hl
        jr      heap_coalesce_loop

        ;; void deallocate(heap_t *h, void *ptr)   HL = h, DE = ptr
_deallocate::
        ld      (__libc_active_heap),hl
        ex      de,hl                   ; HL = ptr
        ld      a,h
        or      l
        ret     z
        push    ix
        call    __libc_heap_unwrap_user
        call    __libc_ptr_to_block
        push    hl
        pop     ix
        ld      BLOCK_FREE_LO(ix),#1
        ld      BLOCK_FREE_HI(ix),#0
        call    __libc_heap_coalesce
        pop     ix
        ret

        ;; void free(void *ptr)   HL = ptr  — owning heap read from the block.
_free::
        ld      a,h
        or      l
        ret     z
        push    ix
        call    __libc_heap_unwrap_user
        call    __libc_ptr_to_block
        push    hl
        pop     ix
        ld      l,BLOCK_HEAP_LO(ix)
        ld      h,BLOCK_HEAP_HI(ix)
        ld      (__libc_active_heap),hl
        ld      BLOCK_FREE_LO(ix),#1
        ld      BLOCK_FREE_HI(ix),#0
        call    __libc_heap_coalesce
        pop     ix
        ret
