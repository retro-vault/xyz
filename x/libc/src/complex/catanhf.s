        ;; catanhf.s
        ;;
        ;; libc catanhf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse hyperbolic tangent via
        ;;   atanh(z) = 0.5 * (clog(1 + z) - clog(1 - z))
        ;; with component-wise subtraction on the complex results.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module catanhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _catanhf
        .globl  _clogf
        .globl  ___fsadd
        .globl  ___fssub
        .globl  ___fsmul

        .area   _CODE

_catanhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-24
        add     hl,sp
        ld      sp,hl

        ;; atanh(0) is exactly 0.
        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        or      8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,catanhf_nonzero
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      sp,ix
        pop     ix
        ret

catanhf_nonzero:

        ;; clog(1 + z)
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
        call    _clogf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      -24(ix),e
        ld      -23(ix),d
        ld      -22(ix),l
        ld      -21(ix),h
        exx
        ld      -20(ix),e
        ld      -19(ix),d
        ld      -18(ix),l
        ld      -17(ix),h
        exx

        ;; clog(1 - z)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        ld      de,#0x0000
        ld      hl,#0x3f80              ; 1.0f
        call    ___fssub
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      a,11(ix)
        xor     #0x80                   ; imag(1-z) = -imag(z)
        ld      h,a
        ld      l,10(ix)
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
        call    _clogf
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        inc     sp
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h
        exx
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h
        exx

        ;; 0.5 * (lp.re - lm.re)
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
        call    ___fssub
        pop     bc
        pop     bc
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

        ;; 0.5 * (lp.im - lm.im)
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        ld      l,-12(ix)
        ld      h,-11(ix)
        push    hl
        ld      e,-20(ix)
        ld      d,-19(ix)
        ld      l,-18(ix)
        ld      h,-17(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      hl,#0x3f00              ; 0.5f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h
        ld      e,-8(ix)
        ld      d,-7(ix)
        ld      l,-6(ix)
        ld      h,-5(ix)
        exx
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        exx
        ld      sp,ix
        pop     ix
        ret
