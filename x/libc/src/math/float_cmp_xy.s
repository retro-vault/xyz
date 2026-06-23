        ;; float_cmp_xy.s
        ;; Split from fmaxf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module float_cmp_xy
        .optsdcc -mz80 sdcccall(1)

        .globl  __float_cmp_xy

        .area   _CODE
__float_cmp_xy::
        ;; treat zero magnitudes as equal-zero first
        ;; sign_x = H bit7, sign_y = 7(ix) bit7
        ld      a,h
        xor     7(ix)
        bit     7,a
        jr      z,fcmp_same_sign
        ;; signs differ: also check both are not zero (so -0 vs +0 == equal)
        ;; magnitude_x == 0 ?
        ld      a,h
        and     #0x7f
        or      l
        or      d
        or      e
        ld      b,a                     ; B != 0 -> x nonzero
        ld      a,7(ix)
        and     #0x7f
        or      6(ix)
        or      5(ix)
        or      4(ix)
        or      b
        jr      z,fcmp_equal            ; both zero -> equal
        ;; differing signs, not both zero: positive one is larger
        bit     7,h
        jr      nz,fcmp_lt              ; x negative -> x < y
        jr      fcmp_gt                ; x positive -> x > y
fcmp_same_sign:
        ;; compare magnitudes (sign bit masked on the top byte), MSB first
        ld      a,h
        and     #0x7f
        ld      b,a
        ld      a,7(ix)
        and     #0x7f
        cp      b
        jr      c,fcmp_mag_x_gt        ; y_top < x_top -> |x| > |y|
        jr      nz,fcmp_mag_x_lt
        ld      a,6(ix)
        cp      l
        jr      c,fcmp_mag_x_gt
        jr      nz,fcmp_mag_x_lt
        ld      a,5(ix)
        cp      d
        jr      c,fcmp_mag_x_gt
        jr      nz,fcmp_mag_x_lt
        ld      a,4(ix)
        cp      e
        jr      c,fcmp_mag_x_gt
        jr      nz,fcmp_mag_x_lt
        jr      fcmp_equal             ; identical magnitudes
fcmp_mag_x_gt:
        ;; |x| > |y|: if both negative x < y, else x > y
        bit     7,h
        jr      nz,fcmp_lt
        jr      fcmp_gt
fcmp_mag_x_lt:
        ;; |x| < |y|: if both negative x > y, else x < y
        bit     7,h
        jr      nz,fcmp_gt
        jr      fcmp_lt
fcmp_gt:
        ld      a,#0x01
        ret
fcmp_lt:
        ld      a,#0xff
        ret
fcmp_equal:
        xor     a
        ret

        ;; __float_load_y
        ;; inputs:  y at 4(ix)..7(ix), IX = frame
        ;; outputs: HL:DE = y  (H=y3, L=y2, D=y1, E=y0)
        ;; clobbers: AF
