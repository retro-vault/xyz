        ;; round_common.s
        ;;
        ;; Shared helpers for the single-precision rounding routines
        ;; (floorf/ceilf/roundf), operating on the float layout HL:DE
        ;; (H=a3 sign/exp, L=a2, D=a1, E=a0).
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih




        .module round_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __float_exp8

        .area   _CODE
__float_exp8::
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        ret     z
        inc     a
        ret

        ;; __float_incmag
        ;; Adds 1 to the magnitude of an integer-valued float.  trunc(+/-0)
        ;; becomes +/-1.0.  Used to round truncated values away from zero.
        ;; inputs:  HL:DE = integer-valued float (exponent < 150)
        ;; outputs: HL:DE = float with |value| increased by one
        ;; clobbers: AF, BC, IX
        ;; frame: -4 sig low, -3 sig mid, -2 sig high (significand bytes)
