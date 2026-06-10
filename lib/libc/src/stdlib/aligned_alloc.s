        ;; aligned_alloc.s
        ;;
        ;; C11 aligned_alloc(alignment, size) built on top of malloc().
        ;; The allocator itself only guarantees 2-byte alignment, so this
        ;; wrapper over-allocates and returns an adjusted user pointer.
        ;;
        ;; Layout immediately ahead of the returned pointer:
        ;;   [-6,-5] requested size (for realloc() copy length)
        ;;   [-4,-3] aligned-allocation magic
        ;;   [-2,-1] original malloc() pointer
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module aligned_alloc
        .optsdcc -mz80 sdcccall(1)

        .globl  _aligned_alloc
        .globl  _malloc

ALIGNED_MAGIC_LO .equ 0x6c
ALIGNED_MAGIC_HI .equ 0xa1
ALIGNED_META_SIZE .equ 6

        .area   _DATA
__libc_aligned_base:
        .dw     0
__libc_aligned_align:
        .dw     0
__libc_aligned_size:
        .dw     0
__libc_aligned_user:
        .dw     0

        .area   _CODE

_aligned_alloc::
        ;; alignment must be non-zero and a power of two
        ld      a,h
        or      l
        jr      z,aligned_alloc_fail
        ld      b,h
        ld      c,l
        dec     bc
        ld      a,h
        and     b
        ld      b,a
        ld      a,l
        and     c
        or      b
        jr      nz,aligned_alloc_fail

        ;; size must be an exact multiple of alignment
        ld      b,h
        ld      c,l
        dec     bc
        ld      a,d
        and     b
        ld      b,a
        ld      a,e
        and     c
        or      b
        jr      nz,aligned_alloc_fail

        ld      (__libc_aligned_align),hl
        ex      de,hl
        ld      (__libc_aligned_size),hl
        ex      de,hl                  ; HL = alignment, DE = size

        ;; total = size + alignment + metadata
        ex      de,hl                  ; HL = size, DE = alignment
        add     hl,de
        jr      c,aligned_alloc_fail
        ld      de,#ALIGNED_META_SIZE
        add     hl,de
        jr      c,aligned_alloc_fail
        call    _malloc
        ld      a,d
        or      e
        jr      z,aligned_alloc_fail

        ld      (__libc_aligned_base),de
        ex      de,hl                  ; HL = base pointer
        ld      de,#ALIGNED_META_SIZE
        add     hl,de                  ; first address after metadata
        ld      de,(__libc_aligned_align)
        dec     de
        add     hl,de                  ; add alignment mask before clearing bits
        ld      bc,(__libc_aligned_align)
        dec     bc
        ld      a,c
        cpl
        ld      c,a
        ld      a,b
        cpl
        ld      b,a
        ld      a,l
        and     c
        ld      l,a
        ld      a,h
        and     b
        ld      h,a
        ld      (__libc_aligned_user),hl

        ;; Write size + magic + base pointer immediately before the user area.
        push    hl
        ld      de,#ALIGNED_META_SIZE
        xor     a
        sbc     hl,de
        ld      de,(__libc_aligned_size)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),#ALIGNED_MAGIC_LO
        inc     hl
        ld      (hl),#ALIGNED_MAGIC_HI
        inc     hl
        ld      de,(__libc_aligned_base)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        pop     hl
        ex      de,hl                  ; DE = aligned user pointer
        ret

aligned_alloc_fail:
        ld      de,#0
        ret
