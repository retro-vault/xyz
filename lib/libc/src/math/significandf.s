        ; significandf.s
        ;
        ; libc significandf implementation for the xcc Z80 libc.
        ; Returns x scaled so the result lies in [1,2) (the significand with
        ; the exponent forced to 0).  Zero is returned unchanged.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module significandf
        .optsdcc -mz80 sdcccall(1)


        .globl  _significandf
        .globl  _significand

        .area   _CODE

        ; _significandf / _significand
        ; inputs:  HL:DE = float x
        ; outputs: HL:DE = x with exponent forced to 0 (value in [1,2)), or 0
        ; clobbers: AF
_significand::
_significandf::
        ; exp8 == 0 -> zero / denormal: return unchanged
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,significandf_chk
        inc     a
significandf_chk:
        or      a
        ret     z
        ; force biased exponent to 127 (0x7F): top bit of L set, H low bit set
        ld      a,h
        and     #0x80
        or      #0x3f                   ; 127 >> 1 = 0x3F
        ld      h,a
        ld      a,l
        or      #0x80                   ; 127 & 1 = 1 -> set L bit7
        ld      l,a
        ret
