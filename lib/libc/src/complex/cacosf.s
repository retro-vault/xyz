        ;; cacosf.s
        ;;
        ;; libc cacosf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse cosine via
        ;;   acos(z) = pi/2 - asin(z)
        ;; with component-wise subtraction.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cacosf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cacosf
        .globl  _casinf
        .globl  ___fssub

        .area   _DATA
__cacosf_asin_re:
        .ds     4
__cacosf_asin_im:
        .ds     4

        .area   _CODE

_cacosf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; acos(1) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        jr      nz,cacosf_generic
        ld      a,6(ix)
        cp      #0x80
        jr      nz,cacosf_generic
        ld      a,7(ix)
        cp      #0x3f
        jr      nz,cacosf_generic
        ld      a,8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,cacosf_generic
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        pop     ix
        ret

cacosf_generic:

        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        call    _casinf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      (__cacosf_asin_re),de
        ld      (__cacosf_asin_re + 2),hl
        exx
        ld      (__cacosf_asin_im),de
        ld      (__cacosf_asin_im + 2),hl
        exx

        ;; real = pi/2 - asin.real
        ld      hl,(__cacosf_asin_re + 2)
        push    hl
        ld      hl,(__cacosf_asin_re)
        push    hl
        ld      de,#0x0fdb
        ld      hl,#0x3fc9              ; pi/2
        call    ___fssub
        pop     bc
        pop     bc

        ;; imag = -asin.imag
        exx
        ld      de,(__cacosf_asin_im)
        ld      hl,(__cacosf_asin_im + 2)
        ld      a,h
        xor     #0x80
        ld      h,a
        exx

        pop     ix
        ret
