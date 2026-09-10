        ; Shared service-list state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _svc_state
        .globl  __svc_first
        .area   _BSS
__svc_first::
        .ds     2
