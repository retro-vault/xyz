        ; feclearexcept.s — clear the given exception flags.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module feclearexcept
        .optsdcc -mz80 sdcccall(1)
        .globl  _feclearexcept
        .globl  __fe_current_env
        .area   _CODE
        ; HL = excepts -> DE = 0
_feclearexcept::
        ld      a,l
        cpl
        ld      c,a
        ld      a,h
        cpl
        ld      b,a                     ; BC = ~excepts
        ld      hl,(__fe_current_env)
        ld      a,l
        and     c
        ld      l,a
        ld      a,h
        and     b
        ld      h,a
        ld      (__fe_current_env),hl
        ld      de,#0
        ret
