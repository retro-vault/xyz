        ;; cargf.s
        ;;
        ;; libc cargf() for the xcc Z80 libc.
        ;; The phase angle is just atan2f(imag, real), so this wrapper rebuilds
        ;; the two stacked float components and forwards them with the standard
        ;; sdcccall(1) float-helper ABI.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cargf
        .optsdcc -mz80 sdcccall(1)

        .globl  _cargf
        .globl  _atan2f

        .area   _CODE

_cargf::
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; Stack the real component as atan2f()'s second argument.
        ld      l,6(ix)
        ld      h,7(ix)
        push    hl
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl

        ;; Load the imaginary component into HL:DE as atan2f()'s first arg.
        ld      e,8(ix)
        ld      d,9(ix)
        ld      l,10(ix)
        ld      h,11(ix)
        call    _atan2f
        pop     af
        pop     af

        pop     ix
        ret
