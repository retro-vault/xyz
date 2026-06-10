        ;; catanf.s
        ;;
        ;; libc catanf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse tangent via
        ;;   atan(z) = -i * atanh(i z)
        ;; mirroring the casinf()/casinhf() relationship.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module catanf
        .optsdcc -mz80 sdcccall(1)

        .globl  _catanf
        .globl  _catanhf

        .area   _DATA
__catanf_atanh_re:
        .ds     4
__catanf_atanh_im:
        .ds     4

        .area   _CODE

_catanf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; atan(0) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        or      8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,catanf_nonzero
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        pop     ix
        ret

catanf_nonzero:

        ;; atanh(i z) with i*(x+iy) = -y + i*x
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      l,10(ix)
        ld      a,11(ix)
        xor     #0x80
        ld      h,a
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        call    _catanhf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      (__catanf_atanh_re),de
        ld      (__catanf_atanh_re + 2),hl
        exx
        ld      (__catanf_atanh_im),de
        ld      (__catanf_atanh_im + 2),hl
        exx

        ;; -i * (u + i v) = v - i u
        ld      de,(__catanf_atanh_im)
        ld      hl,(__catanf_atanh_im + 2)
        exx
        ld      de,(__catanf_atanh_re)
        ld      hl,(__catanf_atanh_re + 2)
        ld      a,h
        xor     #0x80
        ld      h,a
        exx

        pop     ix
        ret
