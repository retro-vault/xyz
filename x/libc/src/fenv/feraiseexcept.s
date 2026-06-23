        ; feraiseexcept.s — set the given exception flags.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module feraiseexcept
        .optsdcc -mz80 sdcccall(1)
        .globl  _feraiseexcept
        .globl  __fe_current_env
        .area   _CODE
        ; HL = excepts -> DE = 0   (FE_ALL_EXCEPT == 0x1F)
_feraiseexcept::
        ld      a,l
        and     #0x1f
        ld      c,a
        ld      a,(__fe_current_env)
        or      c
        ld      (__fe_current_env),a
        ld      de,#0
        ret
