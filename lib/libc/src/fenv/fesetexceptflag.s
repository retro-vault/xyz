        ; fesetexceptflag.s — copy masked flags from *flagp into the env.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fesetexceptflag
        .optsdcc -mz80 sdcccall(1)
        .globl  _fesetexceptflag
        .globl  __fe_current_env
        .area   _CODE
        ; HL = flagp, DE = excepts -> DE = 0 (or 1 if flagp == NULL)
_fesetexceptflag::
        ld      a,h
        or      l
        jr      z,fsef_err
        ld      a,e
        and     #0x1f                   ; C = mask (FE_ALL_EXCEPT, low byte)
        ld      c,a
        ld      a,(hl)
        and     c                       ; B = (*flagp & mask)
        ld      b,a
        ld      a,(__fe_current_env)
        ld      e,a
        ld      a,c
        cpl
        and     e                       ; curlow & ~mask
        or      b
        ld      (__fe_current_env),a
        ld      de,#0
        ret
fsef_err:
        ld      de,#1
        ret
