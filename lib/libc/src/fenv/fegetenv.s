        ; fegetenv.s — store the whole environment into *envp.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fegetenv
        .optsdcc -mz80 sdcccall(1)
        .globl  _fegetenv
        .globl  __fe_current_env
        .area   _CODE
        ; HL = envp -> DE = 0 (or 1 if envp == NULL)
_fegetenv::
        ld      a,h
        or      l
        jr      z,fge_err
        ld      de,#__fe_current_env
        ex      de,hl                   ; HL = current (src), DE = envp (dst)
        ld      bc,#4
        ldir
        ld      de,#0
        ret
fge_err:
        ld      de,#1
        ret
