        ;; libc_isinff.s — 1 if +/-Inf.
        ;; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module libc_isinff
        .optsdcc -mz80 sdcccall(1)
        .globl  __libc_isinff
        .globl  ___libc_isinff
        .globl  __libc_fpclassifyf
        .globl  ___libc_fpclassifyf
        .area   _CODE
__libc_isinff:
___libc_isinff::
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #1                      ; FP_INFINITE
        ld      de,#0
        ret     nz
        inc     de
        ret
