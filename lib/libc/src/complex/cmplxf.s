        ;; cmplxf.s
        ;;
        ;; Internal constructor backing the CMPLXF/CMPLX/CMPLXL macros.
        ;; The compiler's direct complex arithmetic lowering is still not a
        ;; reliable way to build a constant complex value, so the public macro
        ;; funnels through this tiny ABI-stable constructor instead.
        ;;
        ;; sdcccall(1) layout:
        ;;   real float  in DE:HL
        ;;   imag float  at 4(ix)..7(ix)
        ;;
        ;; Complex returns use:
        ;;   DE:HL       = real low/high
        ;;   DE':HL'     = imag low/high
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module cmplxf
        .optsdcc -mz80 sdcccall(1)

        .globl  __cmplxf

        .area   _CODE

__cmplxf:
        push    ix
        ld      ix,#0
        add     ix,sp

        ;; First argument already arrives in the real-component return slots.
        ;; Only load the stacked imaginary component into the alternate bank.
        exx
        ld      e,4(ix)
        ld      d,5(ix)
        ld      l,6(ix)
        ld      h,7(ix)
        exx

        pop     ix
        ret
