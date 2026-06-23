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
AA_FRAME_SIZE .equ 8

        .area   _CODE

AA_USER   .equ 0
AA_SIZE   .equ 2
AA_ALIGN  .equ 4
AA_BASE   .equ 6

_aligned_alloc::
        push    ix
        ld      b,h
        ld      c,l
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        push    hl
        ld      ix,#0
        add     ix,sp
        ld      h,b
        ld      l,c

        ;; alignment must be non-zero and a power of two
        ld      a,h
        or      l
        jp      z,aligned_alloc_fail
        ld      b,h
        ld      c,l
        dec     bc
        ld      a,h
        and     b
        ld      b,a
        ld      a,l
        and     c
        or      b
        jp      nz,aligned_alloc_fail

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
        jp      nz,aligned_alloc_fail

        ld      AA_ALIGN+0(ix),l
        ld      AA_ALIGN+1(ix),h
        ex      de,hl
        ld      AA_SIZE+0(ix),l
        ld      AA_SIZE+1(ix),h
        ex      de,hl                  ; HL = alignment, DE = size

        ;; total = size + alignment + metadata
        ex      de,hl                  ; HL = size, DE = alignment
        add     hl,de
        jp      c,aligned_alloc_fail
        ld      de,#ALIGNED_META_SIZE
        add     hl,de
        jp      c,aligned_alloc_fail
        call    _malloc
        ld      a,d
        or      e
        jp      z,aligned_alloc_fail

        ld      AA_BASE+0(ix),e
        ld      AA_BASE+1(ix),d
        ex      de,hl                  ; HL = base pointer
        ld      de,#ALIGNED_META_SIZE
        add     hl,de                  ; first address after metadata
        ld      e,AA_ALIGN+0(ix)
        ld      d,AA_ALIGN+1(ix)
        dec     de
        add     hl,de                  ; add alignment mask before clearing bits
        ld      c,AA_ALIGN+0(ix)
        ld      b,AA_ALIGN+1(ix)
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
        ld      AA_USER+0(ix),l
        ld      AA_USER+1(ix),h

        ;; Write size + magic + base pointer immediately before the user area.
        push    hl
        ld      de,#ALIGNED_META_SIZE
        xor     a
        sbc     hl,de
        ld      e,AA_SIZE+0(ix)
        ld      d,AA_SIZE+1(ix)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),#ALIGNED_MAGIC_LO
        inc     hl
        ld      (hl),#ALIGNED_MAGIC_HI
        inc     hl
        ld      e,AA_BASE+0(ix)
        ld      d,AA_BASE+1(ix)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        pop     hl
        ex      de,hl                  ; DE = aligned user pointer
        ld      sp,ix
        ld      hl,#AA_FRAME_SIZE
        add     hl,sp
        ld      sp,hl
        pop     ix
        ret

aligned_alloc_fail:
        ld      de,#0
        ld      sp,ix
        ld      hl,#AA_FRAME_SIZE
        add     hl,sp
        ld      sp,hl
        pop     ix
        ret
