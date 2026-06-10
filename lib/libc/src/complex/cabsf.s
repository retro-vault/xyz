        ;; cabsf.s
        ;;
        ;; libc cabsf() for the xcc Z80 libc.
        ;; Computes sqrtf(real*real + imag*imag) using the existing soft-float
        ;; helpers, so the complex support stays a thin layer over the proven
        ;; float runtime.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cabsf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cabsf
        .globl  ___fsmul
        .globl  ___fsadd
        .globl  _sqrtf

        .area   _DATA
__cabsf_real_sq:
        .ds     4

        .area   _CODE

_cabsf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; real*real
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        push    de
        call    ___fsmul
        pop     af
        pop     af
        ld      (__cabsf_real_sq),de
        ld      (__cabsf_real_sq + 2),hl

        ;; imag*imag
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        push    hl
        push    de
        call    ___fsmul
        pop     af
        pop     af

        ;; real*real + imag*imag
        ld      bc,(__cabsf_real_sq)
        push    hl
        push    bc
        call    ___fsadd
        pop     bc
        pop     bc

        ;; sqrtf(sum)
        call    _sqrtf
        pop     ix
        ret
