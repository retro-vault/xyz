        ;; _gpx_neg_hl.s
        ;;
        ;; Shared negation for the advanced shape walkers. Its own archive
        ;; member keeps single-shape programs from pulling unrelated code.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-09-06   TS

        .module _gpx_neg_hl

        .globl  __gpx_neg_hl

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __gpx_neg_hl
        ;; Arguments: HL = signed value
        ;; Return: HL = -HL, modulo 65536
        ;; Clobbers: AF, HL
__gpx_neg_hl::
        xor     a
        sub     l
        ld      l,a
        sbc     a,a
        sub     h
        ld      h,a
        ret
