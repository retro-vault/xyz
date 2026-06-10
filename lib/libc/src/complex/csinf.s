        ;; csinf.s
        ;;
        ;; libc csinf() for the xcc Z80 libc.
        ;; Uses the standard split
        ;;   sin(x + i y) = sin(x) cosh(y) + i cos(x) sinh(y)
        ;; on top of the existing real float kernels.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module csinf
        .optsdcc -mz80 sdcccall(1)

        .globl  _csinf
        .globl  _sinf
        .globl  _cosf
        .globl  _sinhf
        .globl  _coshf
        .globl  ___fsmul

        .area   _DATA
__csinf_sinx:
        .ds     4
__csinf_coshy:
        .ds     4
__csinf_sinhy:
        .ds     4
__csinf_real:
        .ds     4
__csinf_imag:
        .ds     4

        .area   _CODE

_csinf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; sin(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _sinf
        ld      (__csinf_sinx),de
        ld      (__csinf_sinx + 2),hl

        ;; cosh(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _coshf
        ld      (__csinf_coshy),de
        ld      (__csinf_coshy + 2),hl

        ;; real = sin(x) * cosh(y)
        ld      hl,(__csinf_coshy + 2)
        push    hl
        ld      hl,(__csinf_coshy)
        push    hl
        ld      de,(__csinf_sinx)
        ld      hl,(__csinf_sinx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__csinf_real),de
        ld      (__csinf_real + 2),hl

        ;; cos(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _cosf
        ld      (__csinf_sinx),de
        ld      (__csinf_sinx + 2),hl

        ;; sinh(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinhf
        ld      (__csinf_sinhy),de
        ld      (__csinf_sinhy + 2),hl

        ;; imag = cos(x) * sinh(y)
        ld      hl,(__csinf_sinhy + 2)
        push    hl
        ld      hl,(__csinf_sinhy)
        push    hl
        ld      de,(__csinf_sinx)
        ld      hl,(__csinf_sinx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__csinf_imag),de
        ld      (__csinf_imag + 2),hl

        ld      de,(__csinf_real)
        ld      hl,(__csinf_real + 2)
        exx
        ld      de,(__csinf_imag)
        ld      hl,(__csinf_imag + 2)
        exx
        pop     ix
        ret
