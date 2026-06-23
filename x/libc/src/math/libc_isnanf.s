        ;; libc_isnanf.s — 1 if NaN.
        ;; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module libc_isnanf
        .optsdcc -mz80 sdcccall(1)
        .globl  __libc_isnanf
        .globl  ___libc_isnanf
        .globl  __libc_fpclassifyf
        .globl  ___libc_fpclassifyf
        .area   _CODE
__libc_isnanf:
___libc_isnanf::
        call    __libc_fpclassifyf
        ld      a,d
        or      e                       ; FP_NAN == 0 ?
        ld      de,#0
        ret     nz
        inc     de
        ret
