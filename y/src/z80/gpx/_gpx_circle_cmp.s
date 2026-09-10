        ;; _gpx_circle_cmp.s
        ;;
        ;; Common midpoint-circle comparison. Both circle frames keep xn
        ;; and yn at these offsets; no unrelated shape needs this member.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-09-06   TS

        .module _gpx_circle_cmp

        .globl  __gpx_circle_cmp

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __gpx_circle_cmp
        ;; Arguments: IX = circle frame, xn at -5/-6, yn at -7/-8
        ;; Return: HL = xn - yn, flags from the 16-bit subtraction
        ;; Clobbers: AF, DE, HL
__gpx_circle_cmp::
        ld      l,-5(ix)
        ld      h,-6(ix)
        ld      e,-7(ix)
        ld      d,-8(ix)
        or      a
        sbc     hl,de
        ret
