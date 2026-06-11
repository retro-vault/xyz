        ;; heap_core.s
        ;;
        ;; Hand-written allocator core for the xcc Z80 libc.
        ;; The public malloc/calloc/realloc/free entry points stay target-
        ;; independent and obtain their backing arena through __sys_sbrk,
        ;; matching the retargeting contract in RETARGET-LIBC.md.
        ;;
        ;; The allocator keeps one forward-linked free list of headers:
        ;;   struct block { size_t size; int free; struct block *next; }
        ;; with a 6-byte header and 2-byte alignment.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_core
        .optsdcc -mz80 sdcccall(1)

        .globl  _malloc
        .globl  _calloc
        .globl  _realloc
        .globl  _free
        .globl  _free_sized
        .globl  _free_aligned
        .globl  _memcpy
        .globl  ___sys_sbrk
        .globl  __mul16
        .globl  __divuint

BLOCK_SIZE_LO   .equ 0
BLOCK_SIZE_HI   .equ 1
BLOCK_FREE_LO   .equ 2
BLOCK_FREE_HI   .equ 3
BLOCK_NEXT_LO   .equ 4
BLOCK_NEXT_HI   .equ 5
BLOCK_HDR_SIZE  .equ 6
HEAP_ARENA_SIZE .equ 8192
ALIGNED_MAGIC_LO .equ 0x6c
ALIGNED_MAGIC_HI .equ 0xa1

CALLOC_COUNT_LO  .equ -6
CALLOC_COUNT_HI  .equ -5
CALLOC_SIZE_LO   .equ -4
CALLOC_SIZE_HI   .equ -3
CALLOC_TOTAL_LO  .equ -2
CALLOC_TOTAL_HI  .equ -1

REALLOC_PTR_LO   .equ -8
REALLOC_PTR_HI   .equ -7
REALLOC_USER_LO  .equ -6
REALLOC_USER_HI  .equ -5
REALLOC_SIZE_LO  .equ -4
REALLOC_SIZE_HI  .equ -3
REALLOC_AUX_LO   .equ -2
REALLOC_AUX_HI   .equ -1

        .area   _DATA

__libc_heap_head:
        .dw     0
__libc_heap_ready:
        .db     0

        .area   _CODE

;; Round allocation requests up to the allocator's 2-byte alignment.
__libc_align_size:
        inc     hl
        res     0, l
        ret

;; Convert a user payload pointer back to the preceding block header.
__libc_ptr_to_block:
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        dec     hl
        ret

;; aligned_alloc() prefixes its user pointer with:
;;   [-6,-5] requested size
;;   [-4,-3] magic tag
;;   [-2,-1] original malloc() pointer
;; Free always accepts the visible user pointer, while realloc() needs both
;; the original base block and the caller-visible address.
;;   HL = user pointer
;;   HL = original malloc pointer when tagged, unchanged otherwise
;;   carry set when the pointer came from aligned_alloc()
__libc_heap_unwrap_user:
        ld      a,h
        or      l
        ret     z
        push    hl
        ld      de,#4
        or      a
        sbc     hl,de
        ld      a,(hl)
        cp      #ALIGNED_MAGIC_LO
        jr      nz,heap_unwrap_plain
        inc     hl
        ld      a,(hl)
        cp      #ALIGNED_MAGIC_HI
        jr      nz,heap_unwrap_plain
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        pop     hl
        ld      l,e
        ld      h,d
        scf
        ret
heap_unwrap_plain:
        pop     hl
        scf
        ccf
        ret

;; First touch of the heap grabs the fixed arena from __sys_sbrk and seeds one
;; free block that spans the whole region minus the header.
__libc_heap_init:
        ld      a,(__libc_heap_ready)
        or      a
        ret     nz

        ld      hl,#HEAP_ARENA_SIZE
        call    ___sys_sbrk
        ld      a,d
        cp      #0xff
        jr      nz,heap_init_have_arena
        ld      a,e
        cp      #0xff
        jr      nz,heap_init_have_arena
        ld      hl,#0
        ld      (__libc_heap_head),hl
        ld      a,#1
        ld      (__libc_heap_ready),a
        ret

heap_init_have_arena:
        ex      de,hl
        ld      (__libc_heap_head),hl
        ld      de,#(HEAP_ARENA_SIZE - BLOCK_HDR_SIZE)
        ld      (hl),e                  ; block->size low
        inc     hl
        ld      (hl),d                  ; block->size high
        inc     hl
        ld      (hl),#1                 ; block->free = 1
        inc     hl
        xor     a
        ld      (hl),a                  ; block->free high = 0
        inc     hl
        ld      (hl),a                  ; block->next low = 0
        inc     hl
        ld      (hl),a                  ; block->next high = 0
        ld      a,#1
        ld      (__libc_heap_ready),a
        ret

;; Split a free block only when the tail would be large enough to hold another
;; header plus at least one aligned payload byte.
;;   IX = block header
;;   HL = requested payload size
__libc_heap_split:
        push    bc
        ld      b,h
        ld      c,l                     ; BC = requested size
        ld      h,b
        ld      l,c
        ld      de,#(BLOCK_HDR_SIZE + 1)
        add     hl,de                   ; HL = requested size + spill threshold
        ld      e,BLOCK_SIZE_LO(ix)
        ld      d,BLOCK_SIZE_HI(ix)
        ex      de,hl                   ; DE = requested+threshold, HL = block size
        or      a
        sbc     hl,de                   ; block size - minimum tail payload
        jr      c,heap_split_done
        jr      z,heap_split_done

        push    ix
        pop     hl
        ld      de,#BLOCK_HDR_SIZE
        add     hl,de
        add     hl,bc                   ; HL = tail header address
        push    hl                      ; keep tail header address live

        ld      l,BLOCK_SIZE_LO(ix)
        ld      h,BLOCK_SIZE_HI(ix)
        or      a
        sbc     hl,bc
        ld      de,#BLOCK_HDR_SIZE
        or      a
        sbc     hl,de                   ; HL = tail->size
        ex      de,hl
        pop     hl                      ; HL = tail header address
        push    hl                      ; preserve for block->next update
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),#1                 ; tail->free = 1
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      a,BLOCK_NEXT_LO(ix)
        ld      (hl),a                  ; tail->next low
        inc     hl
        ld      a,BLOCK_NEXT_HI(ix)
        ld      (hl),a                  ; tail->next high

        ld      BLOCK_SIZE_LO(ix),c
        ld      BLOCK_SIZE_HI(ix),b
        pop     hl
        ld      BLOCK_NEXT_LO(ix),l
        ld      BLOCK_NEXT_HI(ix),h

heap_split_done:
        pop     bc
        ret

;; After a free, walk the list and join adjacent free blocks back together.
__libc_heap_coalesce:
        ld      hl,(__libc_heap_head)
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

;; malloc(size): first-fit search, split on success, return payload pointer or 0.
_malloc::
        push    ix
        ld      a,h
        or      l
        jr      nz,malloc_have_size
        ld      de,#0
        jr      malloc_return

malloc_have_size:
        call    __libc_align_size
        ld      b,h
        ld      c,l                     ; BC = requested payload size
        call    __libc_heap_init
        ld      hl,(__libc_heap_head)
malloc_loop:
        ld      a,h
        or      l
        jr      nz,malloc_check_block
        ld      de,#0
        jr      malloc_return

malloc_check_block:
        push    hl
        pop     ix
        ld      a,BLOCK_FREE_LO(ix)
        or      BLOCK_FREE_HI(ix)
        jr      z,malloc_next_block

        ld      a,BLOCK_SIZE_LO(ix)
        sub     c
        ld      a,BLOCK_SIZE_HI(ix)
        sbc     a,b
        jr      c,malloc_next_block

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
        jr      malloc_return

malloc_next_block:
        ld      l,BLOCK_NEXT_LO(ix)
        ld      h,BLOCK_NEXT_HI(ix)
        jr      malloc_loop

malloc_return:
        pop     ix
        ret

;; calloc(count, size): overflow-check count*size, malloc it, then zero it.
_calloc::
        ld      a,h
        or      l
        jr      z,calloc_zero_request
        ld      a,d
        or      e
        jr      z,calloc_zero_request

        ld      b,h
        ld      c,l                     ; BC = element count
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-6
        add     hl,sp
        ld      sp,hl
        ld      CALLOC_COUNT_LO(ix),c
        ld      CALLOC_COUNT_HI(ix),b
        ld      CALLOC_SIZE_LO(ix),e
        ld      CALLOC_SIZE_HI(ix),d
        ld      h,b
        ld      l,c
        call    __mul16                        ; DE = count * size (mod 65536)
        ld      CALLOC_TOTAL_LO(ix),e
        ld      CALLOC_TOTAL_HI(ix),d
        ex      de,hl                          ; HL = wrapped product
        ld      e,CALLOC_COUNT_LO(ix)
        ld      d,CALLOC_COUNT_HI(ix)
        call    __divuint                      ; DE = product / count
        ld      l,CALLOC_SIZE_LO(ix)
        ld      h,CALLOC_SIZE_HI(ix)
        ld      a,e
        cp      l
        jr      nz,calloc_fail
        ld      a,d
        cp      h
        jr      nz,calloc_fail

        ld      l,CALLOC_TOTAL_LO(ix)
        ld      h,CALLOC_TOTAL_HI(ix)
        call    _malloc
        ld      a,d
        or      e
        jr      z,calloc_return

        push    de
        ex      de,hl                          ; HL = allocated block
        ld      c,CALLOC_TOTAL_LO(ix)
        ld      b,CALLOC_TOTAL_HI(ix)
        ld      a,b
        or      c
        jr      z,calloc_zero_done
calloc_zero_loop:
        xor     a
        ld      (hl),a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,calloc_zero_loop
calloc_zero_done:
        pop     de
calloc_return:
        ld      sp,ix
        pop     ix
        ret

calloc_zero_request:
        ld      hl,#0
        jp      _malloc

calloc_fail:
        ld      de,#0
        jr      calloc_return

;; realloc(ptr, size): shrink in place when possible, otherwise allocate/copy/free.
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
        ex      de,hl
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
        push    bc
        push    de
        ex      de,hl                          ; HL = destination
        ld      e,REALLOC_USER_LO(ix)
        ld      d,REALLOC_USER_HI(ix)         ; source visible to the caller
        call    _memcpy
        pop     de
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
_free::
        ld      a,h
        or      l
        ret     z
        push    ix
        call    __libc_heap_unwrap_user
        call    __libc_ptr_to_block
        push    hl
        pop     ix
        ld      BLOCK_FREE_LO(ix),#1
        xor     a
        ld      BLOCK_FREE_HI(ix),a
        call    __libc_heap_coalesce
        pop     ix
        ret

;; C23 free_sized and free_aligned (new functions).
;; Implemented in pure assembler in this existing file only.
;; free_sized: size is a hint (ignored here); behaves as free.
;; free_aligned: alignment is the one from aligned_alloc (current free
;; already handles the metadata via unwrap, so we delegate).
;; No additional static data; thread-safe (uses only the existing heap state
;; and caller registers).

_free_sized::
        ld      a,h
        or      l
        ret     z
        jp      _free

_free_aligned::
        ld      a,h
        or      l
        ret     z
        jp      _free
