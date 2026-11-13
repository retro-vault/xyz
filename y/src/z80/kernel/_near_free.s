        ; Release near caller data from common or current-bank memory.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _near_free
        .optsdcc -mz80 sdcccall(1)
        .globl  __near_free
        .globl  __os_free
        .globl  __bank_free
        .globl  __bank_current
        .area   _ROM_EVT

        ; input: HL = live near pointer or zero; output as selected free path.
__near_free::
        ld      a, h
        cp      #0xc0
        jp      c, __os_free
        ld      a, (__bank_current)
        jp      __bank_free
