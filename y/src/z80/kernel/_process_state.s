        ; Shared process state for the YOS kernel.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        ; Process-list roots and public process error status.
        .module _process_state
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_first
        .globl  _process_last_error

        .area   _BSS
_process_first::
        .ds     2
_process_last_error::
        .ds     1
