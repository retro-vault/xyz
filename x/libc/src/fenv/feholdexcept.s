        ; feholdexcept.s — save the env into *envp and clear the flags.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module feholdexcept
        .optsdcc -mz80 sdcccall(1)
        .globl  _feholdexcept
        .globl  __fe_current_env
        .area   _CODE
        ; HL = envp -> DE = 0 (or 1 if envp == NULL)
_feholdexcept::
        ld      a,h
        or      l
        jr      z,fhe_err
        ld      de,#__fe_current_env
        ex      de,hl                   ; HL = current (src), DE = envp (dst)
        ld      bc,#4
        ldir
        ld      hl,#0
        ld      (__fe_current_env),hl   ; clear excepts
        ld      de,#0
        ret
fhe_err:
        ld      de,#1
        ret
