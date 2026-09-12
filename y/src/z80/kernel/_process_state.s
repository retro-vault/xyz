        ; Shared process state for the YOS kernel.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        ; Process-list roots and public process error status.
        .module _process_state
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_first
        .globl  _process_last_error
        .globl  __image_busy
        .globl  __library_refs
        .globl  __library_private_services

        .area   _BSS
_process_first::
        .ds     2
_process_last_error::
        .ds     1
__image_busy::
        .ds     1
__library_refs::
        .ds     2
__library_private_services::
        .ds     2
