        ; libc_signbitf.s — sign bit as 0/1.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module libc_signbitf
        .optsdcc -mz80 sdcccall(1)
        .globl  ___libc_signbitf
        .area   _CODE
___libc_signbitf::
        ld      de,#0
        bit     7,h
        ret     z
        inc     de
        ret
