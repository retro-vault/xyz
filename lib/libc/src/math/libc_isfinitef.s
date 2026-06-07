        ; libc_isfinitef.s — 1 if finite (not Inf/NaN).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module libc_isfinitef
        .optsdcc -mz80 sdcccall(1)
        .globl  ___libc_isfinitef
        .globl  ___libc_fpclassifyf
        .area   _CODE
___libc_isfinitef::
        call    ___libc_fpclassifyf
        ld      a,e                     ; NAN=0,INF=1 -> not finite
        cp      #2
        ld      de,#0
        jr      c,isfin_done
        inc     de
isfin_done:
        ret
