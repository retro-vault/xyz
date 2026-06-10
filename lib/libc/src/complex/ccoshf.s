        ;; ccoshf.s
        ;;
        ;; libc ccoshf() for the xcc Z80 libc.
        ;; Uses the standard split
        ;;   cosh(x + i y) = cosh(x) cos(y) + i sinh(x) sin(y)
        ;; on top of the existing real float kernels.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ccoshf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ccoshf
        .globl  _sinhf
        .globl  _coshf
        .globl  _sinf
        .globl  _cosf
        .globl  ___fsmul

        .area   _DATA
__ccoshf_coshx:
        .ds     4
__ccoshf_sinhx:
        .ds     4
__ccoshf_siny:
        .ds     4
__ccoshf_cosy:
        .ds     4
__ccoshf_real:
        .ds     4
__ccoshf_imag:
        .ds     4

        .area   _CODE

_ccoshf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; cosh(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _coshf
        ld      (__ccoshf_coshx),de
        ld      (__ccoshf_coshx + 2),hl

        ;; cos(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _cosf
        ld      (__ccoshf_cosy),de
        ld      (__ccoshf_cosy + 2),hl

        ;; real = cosh(x) * cos(y)
        ld      hl,(__ccoshf_cosy + 2)
        push    hl
        ld      hl,(__ccoshf_cosy)
        push    hl
        ld      de,(__ccoshf_coshx)
        ld      hl,(__ccoshf_coshx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__ccoshf_real),de
        ld      (__ccoshf_real + 2),hl

        ;; sinh(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _sinhf
        ld      (__ccoshf_sinhx),de
        ld      (__ccoshf_sinhx + 2),hl

        ;; sin(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinf
        ld      (__ccoshf_siny),de
        ld      (__ccoshf_siny + 2),hl

        ;; imag = sinh(x) * sin(y)
        ld      hl,(__ccoshf_siny + 2)
        push    hl
        ld      hl,(__ccoshf_siny)
        push    hl
        ld      de,(__ccoshf_sinhx)
        ld      hl,(__ccoshf_sinhx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__ccoshf_imag),de
        ld      (__ccoshf_imag + 2),hl

        ld      de,(__ccoshf_real)
        ld      hl,(__ccoshf_real + 2)
        exx
        ld      de,(__ccoshf_imag)
        ld      hl,(__ccoshf_imag + 2)
        exx
        pop     ix
        ret
