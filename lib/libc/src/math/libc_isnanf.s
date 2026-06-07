        ; libc_isnanf.s — 1 if NaN.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module libc_isnanf
        .optsdcc -mz80 sdcccall(1)
        .globl  ___libc_isnanf
        .globl  ___libc_fpclassifyf
        .area   _CODE
___libc_isnanf::
        call    ___libc_fpclassifyf
        ld      a,d
        or      e                       ; FP_NAN == 0 ?
        ld      de,#0
        ret     nz
        inc     de
        ret
