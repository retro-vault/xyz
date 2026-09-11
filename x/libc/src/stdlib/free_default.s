        ;; Default-heap free kept separate so hosted platform archives may
        ;; provide free without colliding with deallocate().

        .module free_default
        .optsdcc -mz80 sdcccall(1)

        .globl  _free
        .globl  __libc_heap_coalesce
        .globl  __libc_active_heap
        .globl  __libc_heap_unwrap_user
        .globl  __libc_ptr_to_block

BLOCK_FREE_HI   .equ 3
BLOCK_FREE_LO   .equ 2
BLOCK_HEAP_HI   .equ 7
BLOCK_HEAP_LO   .equ 6

        .area   _CODE
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
