        ; Nesting state shared by kernel critical-section functions.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _critical_state
        .optsdcc -mz80 sdcccall(1)

        .globl  __interrupt_refcount

        .area   _INITIALIZED
__interrupt_refcount::
        .ds     1                       ; bit 7 saved IFF, bits 0..6 depth

        .area   _INITIALIZER
        .db     0
