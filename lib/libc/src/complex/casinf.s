        ;; casinf.s
        ;;
        ;; libc casinf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse sine via
        ;;   asin(z) = -i * asinh(i z)
        ;; so this wrapper only needs to rotate the components into an
        ;; imaginary-axis input, call casinhf(), then rotate back.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module casinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _casinf
        .globl  _casinhf

        .area   _DATA
__casinf_asinh_re:
        .ds     4
__casinf_asinh_im:
        .ds     4

        .area   _CODE

_casinf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; asin(0) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        or      8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,casinf_nonzero
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        pop     ix
        ret

casinf_nonzero:

        ;; asinh(i z) with i*(x+iy) = -y + i*x
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
        call    _casinhf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      (__casinf_asinh_re),de
        ld      (__casinf_asinh_re + 2),hl
        exx
        ld      (__casinf_asinh_im),de
        ld      (__casinf_asinh_im + 2),hl
        exx

        ;; -i * (u + i v) = v - i u
        ld      de,(__casinf_asinh_im)
        ld      hl,(__casinf_asinh_im + 2)
        exx
        ld      de,(__casinf_asinh_re)
        ld      hl,(__casinf_asinh_re + 2)
        ld      a,h
        xor     #0x80
        ld      h,a
        exx

        pop     ix
        ret
