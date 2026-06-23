        ; feupdateenv.s — install *envp then re-raise the previously-set flags.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module feupdateenv
        .optsdcc -mz80 sdcccall(1)
        .globl  _feupdateenv
        .globl  _fesetenv
        .globl  __fe_current_env
        .area   _CODE
        ; HL = envp -> DE = 0 on success, 1 on failure
_feupdateenv::
        ld      a,h
        or      l
        jr      z,fue_err
        ld      bc,(__fe_current_env)   ; BC = pending excepts
        push    bc
        call    _fesetenv               ; HL = envp
        ld      a,d
        or      e
        jr      nz,fue_fail
        pop     bc
        ld      hl,(__fe_current_env)
        ld      a,l
        or      c
        ld      l,a
        ld      a,h
        or      b
        ld      h,a
        ld      (__fe_current_env),hl
        ld      de,#0
        ret
fue_fail:
        pop     bc
        ld      de,#1
        ret
fue_err:
        ld      de,#1
        ret
