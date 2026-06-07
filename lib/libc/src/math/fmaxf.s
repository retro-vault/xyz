        ; fmaxf.s
        ;
        ; libc fmaxf implementation for the xcc Z80 libc.
        ; Returns the larger of two single-precision values.  NaN inputs are
        ; not handled specially (this runtime has no NaN support).  Shares the
        ; sign/magnitude comparison core with fminf.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fmaxf
        .optsdcc -mz80 sdcccall(1)


        .globl  _fmaxf
        .globl  _fmax
        .globl  _fmaxl
        .globl  __float_cmp_xy
        .globl  __float_load_y

        .area   _CODE

        ; _fmaxf / _fmax / _fmaxl  (32-bit float types on this target)
        ; inputs:  HL:DE = x, y on stack (4(ix)..7(ix))
        ; outputs: HL:DE = max(x, y)
        ; clobbers: AF, BC, IX
_fmax::
_fmaxl::
_fmaxf::
        push    ix
        ld      ix,#0
        add     ix,sp
        call    __float_cmp_xy          ; A = +1 (x>y), 0 (==), 0xFF (x<y)
        inc     a                       ; 0xFF -> 0 when x < y
        jr      nz,fmaxf_keep_x
        call    __float_load_y          ; x < y: return y
fmaxf_keep_x:
        pop     ix
        ret

        ; __float_cmp_xy
        ; inputs:  HL:DE = x, y at 4(ix)..7(ix), IX = frame
        ; outputs: A = sign of (x - y): 0x01 if x>y, 0x00 if equal, 0xFF if x<y
        ; clobbers: AF, BC
        ; notes: sign/magnitude ordering; +/-0 compare equal.
__float_cmp_xy::
        ; treat zero magnitudes as equal-zero first
        ; sign_x = H bit7, sign_y = 7(ix) bit7
        ld      a,h
        xor     7(ix)
        bit     7,a
        jr      z,fcmp_same_sign
        ; signs differ: also check both are not zero (so -0 vs +0 == equal)
        ; magnitude_x == 0 ?
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
        ; differing signs, not both zero: positive one is larger
        bit     7,h
        jr      nz,fcmp_lt              ; x negative -> x < y
        jr      fcmp_gt                ; x positive -> x > y
fcmp_same_sign:
        ; compare magnitudes (sign bit masked on the top byte), MSB first
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
        ; |x| > |y|: if both negative x < y, else x > y
        bit     7,h
        jr      nz,fcmp_lt
        jr      fcmp_gt
fcmp_mag_x_lt:
        ; |x| < |y|: if both negative x > y, else x < y
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

        ; __float_load_y
        ; inputs:  y at 4(ix)..7(ix), IX = frame
        ; outputs: HL:DE = y  (H=y3, L=y2, D=y1, E=y0)
        ; clobbers: AF
__float_load_y::
        ld      a,7(ix)
        ld      h,a
        ld      a,6(ix)
        ld      l,a
        ld      a,5(ix)
        ld      d,a
        ld      a,4(ix)
        ld      e,a
        ret
