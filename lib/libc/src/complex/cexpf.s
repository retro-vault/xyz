        ;; cexpf.s
        ;;
        ;; libc cexpf() for the xcc Z80 libc.
        ;; Uses the identity
        ;;   exp(x + i y) = exp(x) * (cos(y) + i sin(y))
        ;; on top of the existing real-valued float helpers.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cexpf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cexpf
        .globl  _expf
        .globl  _cosf
        .globl  _sinf
        .globl  ___fsmul

        .area   _DATA
__cexpf_exp:
        .ds     4
__cexpf_real:
        .ds     4
__cexpf_imag:
        .ds     4

        .area   _CODE

_cexpf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; expx = expf(real(z))
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        call    _expf
        ld      (__cexpf_exp),de
        ld      (__cexpf_exp + 2),hl

        ;; real = expx * cosf(imag(z))
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _cosf
        push    hl
        push    de
        ld      de,(__cexpf_exp)
        ld      hl,(__cexpf_exp + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cexpf_real),de
        ld      (__cexpf_real + 2),hl

        ;; imag = expx * sinf(imag(z))
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _sinf
        push    hl
        push    de
        ld      de,(__cexpf_exp)
        ld      hl,(__cexpf_exp + 2)
        call    ___fsmul
        pop     bc
        pop     bc
        ld      (__cexpf_imag),de
        ld      (__cexpf_imag + 2),hl

        ;; Return the packed complex result.
        ld      de,(__cexpf_real)
        ld      hl,(__cexpf_real + 2)
        exx
        ld      de,(__cexpf_imag)
        ld      hl,(__cexpf_imag + 2)
        exx

        pop     ix
        ret
