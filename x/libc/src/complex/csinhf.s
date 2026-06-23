        ;; csinhf.s
        ;;
        ;; libc csinhf() for the xcc Z80 libc.
        ;; Uses the standard split
        ;;   sinh(x + i y) = sinh(x) cos(y) + i cosh(x) sin(y)
        ;; on top of the existing real float kernels.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module csinhf
        .optsdcc -mz80 sdcccall(1)

        .globl  _csinhf
        .globl  _sinhf
        .globl  _coshf
        .globl  _sinf
        .globl  _cosf
        .globl  ___fsmul

        .area   _CODE

_csinhf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-24
        add     hl,sp
        ld      sp,hl

        ;; sinh(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _sinhf
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h

        ;; cos(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _cosf
        ld      -8(ix),e
        ld      -7(ix),d
        ld      -6(ix),l
        ld      -5(ix),h

        ;; real = sinh(x) * cos(y)
        ld      l,-6(ix)
        ld      h,-5(ix)
        push    hl
        ld      l,-8(ix)
        ld      h,-7(ix)
        push    hl
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -20(ix),e
        ld      -19(ix),d
        ld      -18(ix),l
        ld      -17(ix),h

        ;; cosh(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _coshf
        ld      -12(ix),e
        ld      -11(ix),d
        ld      -10(ix),l
        ld      -9(ix),h

        ;; sin(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinf
        ld      -16(ix),e
        ld      -15(ix),d
        ld      -14(ix),l
        ld      -13(ix),h

        ;; imag = cosh(x) * sin(y)
        ld      l,-14(ix)
        ld      h,-13(ix)
        push    hl
        ld      l,-16(ix)
        ld      h,-15(ix)
        push    hl
        ld      e,-12(ix)
        ld      d,-11(ix)
        ld      l,-10(ix)
        ld      h,-9(ix)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      -24(ix),e
        ld      -23(ix),d
        ld      -22(ix),l
        ld      -21(ix),h

        ld      e,-20(ix)
        ld      d,-19(ix)
        ld      l,-18(ix)
        ld      h,-17(ix)
        exx
        ld      e,-24(ix)
        ld      d,-23(ix)
        ld      l,-22(ix)
        ld      h,-21(ix)
        exx
        ld      sp,ix
        pop     ix
        ret
