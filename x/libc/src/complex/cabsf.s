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

        .area   _CODE

_cabsf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-4
        add     hl,sp
        ld      sp,hl

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
        ld      -4(ix),e
        ld      -3(ix),d
        ld      -2(ix),l
        ld      -1(ix),h

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
        push    hl
        push    de
        ld      e,-4(ix)
        ld      d,-3(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    ___fsadd
        pop     bc
        pop     bc

        ;; sqrtf(sum)
        call    _sqrtf
        ld      sp,ix
        pop     ix
        ret
