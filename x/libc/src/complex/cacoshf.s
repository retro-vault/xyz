        ;; cacoshf.s
        ;;
        ;; libc cacoshf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse hyperbolic cosine via
        ;;   acosh(z) = clog(z + csqrt(z + 1) * csqrt(z - 1))
        ;; using the existing square-root/log kernels plus explicit complex
        ;; multiplication.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cacoshf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cacoshf
        .globl  _csqrtf
        .globl  _clogf
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul

        .area   _CODE

_cacoshf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      bc,#-28
        add     ix,bc
        ld      sp,ix
        ld      bc,#28
        add     ix,bc

        ;; acosh(1) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        jr      nz,cacoshf_generic
        ld      a,6(ix)
        cp      #0x80
        jr      nz,cacoshf_generic
        ld      a,7(ix)
        cp      #0x3f
        jr      nz,cacoshf_generic
        ld      a,8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,cacoshf_generic
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      sp,ix
        pop     ix
        ret

cacoshf_generic:

        ;; a = csqrt(z + 1)
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        call    _csqrtf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      -28(ix),e
        ld      -27(ix),d
        ld      -26(ix),l
        ld      -25(ix),h
        exx
        ld      -24(ix),e
        ld      -23(ix),d
        ld      -22(ix),l
        ld      -21(ix),h
        exx

        ;; b = csqrt(z - 1)
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        ld      l,8(ix)
        ld      h,9(ix)
        push    hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        call    _csqrtf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      -20(ix),e
        ld      -19(ix),d
        ld      -18(ix),l
        ld      -17(ix),h
        exx
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h
        exx

        ;; prod.re = a.re*b.re - a.im*b.im
        ld      l,-18(ix)
        ld      h,-17(ix)
        push    hl
        ld      l,-20(ix)
        ld      h,-19(ix)
        push    hl
        ld      e,-28(ix)
        ld      d,-27(ix)
        ld      l,-26(ix)
        ld      h,-25(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,-14(ix)
        ld      h,-13(ix)
        push    hl
        ld      l,-16(ix)
        ld      h,-15(ix)
        push    hl
        ld      e,-24(ix)
        ld      d,-23(ix)
        ld      l,-22(ix)
        ld      h,-21(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; prod.im = a.re*b.im + a.im*b.re
        ld      l,-14(ix)
        ld      h,-13(ix)
        push    hl
        ld      l,-16(ix)
        ld      h,-15(ix)
        push    hl
        ld      e,-28(ix)
        ld      d,-27(ix)
        ld      l,-26(ix)
        ld      h,-25(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,-18(ix)
        ld      h,-17(ix)
        push    hl
        ld      l,-20(ix)
        ld      h,-19(ix)
        push    hl
        ld      e,-24(ix)
        ld      d,-23(ix)
        ld      l,-22(ix)
        ld      h,-21(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        push    hl
        push    de
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

        ;; clog(z + prod)
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        ld      l,-12(ix)
        ld      h,-11(ix)
        push    hl
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        call    _clogf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp

        ld      sp,ix
        pop     ix
        ret
