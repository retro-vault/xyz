        ; logbf.s
        ;
        ; libc logbf implementation for the xcc Z80 libc.
        ; Returns the unbiased exponent of x as a floating-point value
        ; (FLT_RADIX == 2, so logb == ilogb expressed as a float).
        ; logb(0) returns -infinity.  double / long double are 32-bit on
        ; this target, so logb / logbl share the implementation.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module logbf
        .optsdcc -mz80 sdcccall(1)


        .globl  _logbf
        .globl  _logb
        .globl  _logbl
        .globl  __float_exp8
        .globl  ___sint2fs

        .area   _CODE

        ; _logbf / _logb / _logbl
        ; inputs:  HL:DE = float x
        ; outputs: HL:DE = (float)floor(log2(|x|)), or -Inf for x == 0
        ; clobbers: AF, BC, DE, HL
_logb::
_logbl::
_logbf::
        call    __float_exp8            ; A = exp8 (HL:DE preserved)
        or      a
        jr      z,logbf_zero
        sub     #127                    ; A = unbiased exponent (signed)
        ld      l,a
        rla
        sbc     a,a                     ; sign-extend into H
        ld      h,a
        jp      ___sint2fs              ; HL = int16 -> HL:DE = float
logbf_zero:
        ld      h,#0xff                 ; -Inf = 0xFF800000
        ld      l,#0x80
        ld      d,#0
        ld      e,#0
        ret
