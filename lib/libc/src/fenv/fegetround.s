        ; fegetround.s — return the current rounding mode.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fegetround
        .optsdcc -mz80 sdcccall(1)
        .globl  _fegetround
        .globl  __fe_current_env
        .area   _CODE
_fegetround::
        ld      de,(__fe_current_env + 2)
        ret
