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

        .area   _DATA
__csinhf_sinhx:
        .ds     4
__csinhf_coshx:
        .ds     4
__csinhf_siny:
        .ds     4
__csinhf_cosy:
        .ds     4
__csinhf_real:
        .ds     4
__csinhf_imag:
        .ds     4

        .area   _CODE

_csinhf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; sinh(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _sinhf
        ld      (__csinhf_sinhx),de
        ld      (__csinhf_sinhx + 2),hl

        ;; cos(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _cosf
        ld      (__csinhf_cosy),de
        ld      (__csinhf_cosy + 2),hl

        ;; real = sinh(x) * cos(y)
        ld      hl,(__csinhf_cosy + 2)
        push    hl
        ld      hl,(__csinhf_cosy)
        push    hl
        ld      de,(__csinhf_sinhx)
        ld      hl,(__csinhf_sinhx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__csinhf_real),de
        ld      (__csinhf_real + 2),hl

        ;; cosh(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _coshf
        ld      (__csinhf_coshx),de
        ld      (__csinhf_coshx + 2),hl

        ;; sin(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinf
        ld      (__csinhf_siny),de
        ld      (__csinhf_siny + 2),hl

        ;; imag = cosh(x) * sin(y)
        ld      hl,(__csinhf_siny + 2)
        push    hl
        ld      hl,(__csinhf_siny)
        push    hl
        ld      de,(__csinhf_coshx)
        ld      hl,(__csinhf_coshx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__csinhf_imag),de
        ld      (__csinhf_imag + 2),hl

        ld      de,(__csinhf_real)
        ld      hl,(__csinhf_real + 2)
        exx
        ld      de,(__csinhf_imag)
        ld      hl,(__csinhf_imag + 2)
        exx
        pop     ix
        ret
