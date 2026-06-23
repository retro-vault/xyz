        ; fetestexcept.s — return the env flags masked by excepts.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fetestexcept
        .optsdcc -mz80 sdcccall(1)
        .globl  _fetestexcept
        .globl  __fe_current_env
        .area   _CODE
        ; HL = excepts -> DE = excepts & current flags
_fetestexcept::
        ld      bc,(__fe_current_env)
        ld      a,c
        and     l
        ld      e,a
        ld      a,b
        and     h
        ld      d,a
        ret
