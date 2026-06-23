        ;; realloc.s
        ;; Split from heap_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module realloc
        .optsdcc -mz80 sdcccall(1)

        .globl  _realloc
        .globl  __libc_align_size
        .globl  __libc_heap_split
        .globl  __libc_heap_unwrap_user
        .globl  __libc_ptr_to_block
        .globl  _free
        .globl  _malloc
        .globl  _memcpy

BLOCK_SIZE_HI   .equ 1
BLOCK_SIZE_LO   .equ 0
REALLOC_AUX_HI   .equ -1
REALLOC_AUX_LO   .equ -2
REALLOC_PTR_HI   .equ -7
REALLOC_PTR_LO   .equ -8
REALLOC_SIZE_HI  .equ -3
REALLOC_SIZE_LO  .equ -4
REALLOC_USER_HI  .equ -5
REALLOC_USER_LO  .equ -6

        .area   _CODE
_realloc::
        ld      a,h
        or      l
        jr      nz,realloc_have_ptr
        ex      de,hl
        jp      _malloc

realloc_have_ptr:
        ld      b,h
        ld      c,l
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-8
        add     hl,sp
        ld      sp,hl
        ld      REALLOC_PTR_LO(ix),c
        ld      REALLOC_PTR_HI(ix),b
        ld      REALLOC_USER_LO(ix),c
        ld      REALLOC_USER_HI(ix),b
        ld      REALLOC_SIZE_LO(ix),e
        ld      REALLOC_SIZE_HI(ix),d
        ld      a,d
        or      e
        jr      nz,realloc_have_size
        ld      l,REALLOC_PTR_LO(ix)
        ld      h,REALLOC_PTR_HI(ix)
        call    _free
        ld      de,#0
        jp      realloc_return

realloc_have_size:
        ld      l,REALLOC_PTR_LO(ix)
        ld      h,REALLOC_PTR_HI(ix)
        call    __libc_heap_unwrap_user
        jp      c,realloc_aligned_ptr
        push    hl
        ld      l,REALLOC_SIZE_LO(ix)
        ld      h,REALLOC_SIZE_HI(ix)
        call    __libc_align_size
        ld      REALLOC_SIZE_LO(ix),l
        ld      REALLOC_SIZE_HI(ix),h
        pop     hl
        call    __libc_ptr_to_block
        ld      e,REALLOC_SIZE_LO(ix)
        ld      d,REALLOC_SIZE_HI(ix)
        ld      b,d
        ld      c,e
        push    ix
        push    hl
        pop     ix
        ld      a,BLOCK_SIZE_LO(ix)
        sub     e
        ld      a,BLOCK_SIZE_HI(ix)
        sbc     a,d
        jr      c,realloc_allocate_new_with_block

        ld      h,b
        ld      l,c
        call    __libc_heap_split
        pop     ix
        ld      e,REALLOC_PTR_LO(ix)
        ld      d,REALLOC_PTR_HI(ix)
        jp      realloc_return

realloc_allocate_new_with_block:
        ld      l,BLOCK_SIZE_LO(ix)
        ld      h,BLOCK_SIZE_HI(ix)
        pop     ix
        ld      REALLOC_AUX_LO(ix),l
        ld      REALLOC_AUX_HI(ix),h           ; old payload length
realloc_allocate_common:
        ld      l,REALLOC_SIZE_LO(ix)
        ld      h,REALLOC_SIZE_HI(ix)
        call    _malloc
        ld      a,d
        or      e
        jr      z,realloc_return
        push    de                              ; keep the newly allocated block

        ld      l,REALLOC_AUX_LO(ix)
        ld      h,REALLOC_AUX_HI(ix)
        ld      e,REALLOC_SIZE_LO(ix)
        ld      d,REALLOC_SIZE_HI(ix)
        xor     a
        sbc     hl,de
        jr      c,realloc_copy_old
        ld      c,REALLOC_SIZE_LO(ix)
        ld      b,REALLOC_SIZE_HI(ix)
        jr      realloc_copy_have_len
realloc_copy_old:
        ld      c,REALLOC_AUX_LO(ix)
        ld      b,REALLOC_AUX_HI(ix)
realloc_copy_have_len:
        pop     hl                              ; HL = destination
        ld      e,REALLOC_USER_LO(ix)
        ld      d,REALLOC_USER_HI(ix)         ; source visible to the caller
        push    bc                              ; memcpy byte count argument
        call    _memcpy
        pop     bc
        push    de
        ld      l,REALLOC_PTR_LO(ix)
        ld      h,REALLOC_PTR_HI(ix)
        call    _free
        pop     de
        jp      realloc_return

realloc_aligned_ptr:
        ld      l,REALLOC_USER_LO(ix)
        ld      h,REALLOC_USER_HI(ix)
        ld      de,#6
        or      a
        sbc     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      REALLOC_AUX_LO(ix),e
        ld      REALLOC_AUX_HI(ix),d           ; old user-visible size
        jp      realloc_allocate_common

realloc_return:
        ld      sp,ix
        pop     ix
        ret

;; free(ptr): mark the block free and eagerly coalesce adjacent blocks.
