        ;; significandf.s
        ;;
        ;; libc significandf implementation for the xcc Z80 libc.
        ;; Returns x scaled so the result lies in [1,2) (the significand with
        ;; the exponent forced to 0).  Zero is returned unchanged.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module significandf
        .optsdcc -mz80 sdcccall(1)


        .globl  _significandf
        .area   _CODE

        ;; HL:DE carries an IEEE-754 single.
        ;; Rewriting the biased exponent to 127 normalizes the value into
        ;; [1,2) while preserving the original sign and fraction bits.
_significandf::
        ;; Reconstruct the 8-bit biased exponent from H/L. Zero and subnormal
        ;; inputs keep exp8 == 0, and those are returned unchanged.
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,significandf_chk
        inc     a
significandf_chk:
        or      a
        ret     z
        ;; Write back biased exponent 127 (0x7F) without disturbing the sign
        ;; bit in H or the mantissa bits in L:DE.
        ld      a,h
        and     #0x80
        or      #0x3f                   ; 127 >> 1 = 0x3F
        ld      h,a
        ld      a,l
        or      #0x80                   ; 127 & 1 = 1 -> set L bit7
        ld      l,a
        ret
