        ;; casinhf.s
        ;;
        ;; libc casinhf() for the xcc Z80 libc.
        ;;
        ;; Principal-value inverse hyperbolic sine via
        ;;   asinh(z) = clog(z + csqrt(z*z + 1))
        ;; built from the existing real float helpers plus csqrtf()/clogf().
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module casinhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _casinhf
        .globl  _casinh
        .globl  _casinhl
        .globl  _csqrtf
        .globl  _clogf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  ___fssub

        .area   _CODE

_casinhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-28
        add     hl,sp
        ld      sp,hl

        ;; asinh(0) is exactly 0, and short-circuiting here avoids feeding the
        ;; soft-float log kernel its most cancellation-prone identity case.
        ld      a,4(ix)
        or      5(ix)
        or      6(ix)
        or      7(ix)
        or      8(ix)
        or      9(ix)
        or      10(ix)
        or      11(ix)
        jr      nz,casinhf_nonzero
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      de,#0x0000
        ld      hl,#0x0000
        exx
        ld      sp,ix
        pop     ix
        ret

casinhf_nonzero:

        ;; x^2
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        push    de
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -28(ix),e
        ld      -27(ix),d
        ld      -26(ix),l
        ld      -25(ix),h

        ;; y^2
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        push    de
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -24(ix),e
        ld      -23(ix),d
        ld      -22(ix),l
        ld      -21(ix),h

        ;; real(z*z + 1) = x^2 - y^2 + 1
        ld      l,-22(ix)
        ld      h,-21(ix)
        push    hl
        ld      l,-24(ix)
        ld      h,-23(ix)
        push    hl
        ld      e,-28(ix)
        ld      d,-27(ix)
        ld      l,-26(ix)
        ld      h,-25(ix)
        call    ___fssub
        pop     bc
        pop     bc
        ld      hl,#0x3f80              ; 1.0f
        push    hl
        ld      hl,#0x0000
        push    hl
        call    ___fsadd
        pop     bc
        pop     bc
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; imag(z*z + 1) = 2*x*y
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        push    de
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      hl,#0x4000              ; 2.0f
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

        ;; s = csqrtf(z*z + 1)
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl
        ld      l,-10(ix)
        ld      h,-9(ix)
        push    hl
        ld      l,-12(ix)
        ld      h,-11(ix)
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

        ;; arg = z + s
        ld      l,-18(ix)
        ld      h,-17(ix)
        push    hl
        ld      l,-20(ix)
        ld      h,-19(ix)
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

        ld      l,-14(ix)
        ld      h,-13(ix)
        push    hl
        ld      l,-16(ix)
        ld      h,-15(ix)
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

        ;; clogf(arg)
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

;; C23 double/long double aliases for complex inverse (new, in existing file).
;; Basic forward to f version (consistent with other float-first in complex and math).
_casinh::
_casinhl::
        jp      _casinhf
