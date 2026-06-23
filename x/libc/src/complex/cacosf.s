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

        ;; The alternate bank already holds asin.imag, so negate it there
        ;; before computing pi/2 - asin.real in the primary bank.
        exx
        ld      a,h
        xor     #0x80
        ld      h,a
        exx

        ;; real = pi/2 - asin.real
        push    hl
        push    de
        ld      de,#0x0fdb
        ld      hl,#0x3fc9              ; pi/2
        call    ___fssub
        pop     bc
        pop     bc

        pop     ix
        ret
