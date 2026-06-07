        ; libc_fpclassifyf.s — IEEE-754 single classification core.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module libc_fpclassifyf
        .optsdcc -mz80 sdcccall(1)
        .globl  ___libc_fpclassifyf
        .area   _CODE
        ; HL:DE = x -> DE = FP_NAN/INFINITE/ZERO/SUBNORMAL/NORMAL
        ; preserves HL
___libc_fpclassifyf::
        ld      a,h
        and     #0x7f
        add     a,a
        bit     7,l
        jr      z,fpc_e
        inc     a
fpc_e:
        ld      c,a                     ; C = exp8
        ld      a,l
        and     #0x7f
        or      d
        or      e
        ld      b,a                     ; B = fraction (nonzero?)
        ld      a,c
        cp      #0xff
        jr      z,fpc_inf_nan
        or      a
        jr      z,fpc_zero_sub
        ld      de,#4                   ; FP_NORMAL
        ret
fpc_inf_nan:
        ld      a,b
        or      a
        jr      z,fpc_inf
        ld      de,#0                   ; FP_NAN
        ret
fpc_inf:
        ld      de,#1                   ; FP_INFINITE
        ret
fpc_zero_sub:
        ld      a,b
        or      a
        jr      z,fpc_zero
        ld      de,#3                   ; FP_SUBNORMAL
        ret
fpc_zero:
        ld      de,#2                   ; FP_ZERO
        ret
