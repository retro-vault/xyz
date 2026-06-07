        ; fegetexceptflag.s — read the masked exception flags into *flagp.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fegetexceptflag
        .optsdcc -mz80 sdcccall(1)
        .globl  _fegetexceptflag
        .globl  __fe_current_env
        .area   _CODE
        ; HL = flagp, DE = excepts -> DE = 0 (or 1 if flagp == NULL)
_fegetexceptflag::
        ld      a,h
        or      l
        jr      z,fgef_err
        ld      bc,(__fe_current_env)   ; BC = excepts field
        ld      a,c
        and     e
        ld      (hl),a
        inc     hl
        ld      a,b
        and     d
        ld      (hl),a
        ld      de,#0
        ret
fgef_err:
        ld      de,#1
        ret
