        ; fesetenv.s — install *envp as the environment (flags masked).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fesetenv
        .optsdcc -mz80 sdcccall(1)
        .globl  _fesetenv
        .globl  __fe_current_env
        .area   _CODE
        ; HL = envp -> DE = 0 on success, 1 on NULL/invalid rounding
_fesetenv::
        ld      a,h
        or      l
        jr      z,fse_err
        push    hl
        inc     hl
        inc     hl
        ld      a,(hl)                  ; rounding low
        ld      c,a
        inc     hl
        ld      a,(hl)                  ; rounding high
        or      a
        jr      nz,fse_err_pop
        ld      a,c
        cp      #4
        jr      nc,fse_err_pop
        pop     hl                      ; HL = envp (src)
        ld      de,#__fe_current_env
        ld      bc,#4
        ldir
        ld      a,(__fe_current_env)
        and     #0x1f
        ld      (__fe_current_env),a
        xor     a
        ld      (__fe_current_env + 1),a
        ld      de,#0
        ret
fse_err_pop:
        pop     hl
fse_err:
        ld      de,#1
        ret
