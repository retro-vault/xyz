        ;; ccosf.s
        ;;
        ;; libc ccosf() for the xcc Z80 libc.
        ;; Uses the standard split
        ;;   cos(x + i y) = cos(x) cosh(y) - i sin(x) sinh(y)
        ;; on top of the existing real float kernels.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module ccosf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ccosf
        .globl  _sinf
        .globl  _cosf
        .globl  _sinhf
        .globl  _coshf
        .globl  ___fsmul

        .area   _DATA
__ccosf_cosx:
        .ds     4
__ccosf_coshy:
        .ds     4
__ccosf_sinx:
        .ds     4
__ccosf_sinhy:
        .ds     4
__ccosf_real:
        .ds     4
__ccosf_imag:
        .ds     4

        .area   _CODE

_ccosf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; cos(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _cosf
        ld      (__ccosf_cosx),de
        ld      (__ccosf_cosx + 2),hl

        ;; cosh(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _coshf
        ld      (__ccosf_coshy),de
        ld      (__ccosf_coshy + 2),hl

        ;; real = cos(x) * cosh(y)
        ld      hl,(__ccosf_coshy + 2)
        push    hl
        ld      hl,(__ccosf_coshy)
        push    hl
        ld      de,(__ccosf_cosx)
        ld      hl,(__ccosf_cosx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__ccosf_real),de
        ld      (__ccosf_real + 2),hl

        ;; sin(x)
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _sinf
        ld      (__ccosf_sinx),de
        ld      (__ccosf_sinx + 2),hl

        ;; sinh(y)
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinhf
        ld      (__ccosf_sinhy),de
        ld      (__ccosf_sinhy + 2),hl

        ;; imag = -(sin(x) * sinh(y))
        ld      hl,(__ccosf_sinhy + 2)
        push    hl
        ld      hl,(__ccosf_sinhy)
        push    hl
        ld      de,(__ccosf_sinx)
        ld      hl,(__ccosf_sinx + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      a,h
        xor     #0x80
        ld      h,a
        ld      (__ccosf_imag),de
        ld      (__ccosf_imag + 2),hl

        ld      de,(__ccosf_real)
        ld      hl,(__ccosf_real + 2)
        exx
        ld      de,(__ccosf_imag)
        ld      hl,(__ccosf_imag + 2)
        exx
        pop     ix
        ret
