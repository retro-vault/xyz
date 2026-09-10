        ; Shared YOS syscall-table storage.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _yos_state
        .globl  __yos
        .area   _BSS
__yos::
        .ds     94
