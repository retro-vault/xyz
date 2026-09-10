        ; Shared timer-list state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _tmr_state
        .globl  __tmr_first
        .area   _BSS
__tmr_first::
        .ds     2
