        ; Shared event-list state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _evt_state
        .globl  __evt_first
        .area   _BSS
__evt_first::
        .ds     2
