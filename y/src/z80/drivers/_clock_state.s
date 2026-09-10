        ; Shared clock counters.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _clock_state
        .globl  _clock_ticks
        .globl  _clock_time
        .globl  _clock_sec_countdown
        .area   _INITIALIZED
_clock_ticks::
        .ds     4
_clock_time::
        .ds     4
_clock_sec_countdown::
        .ds     1
        .area   _INITIALIZER
        .byte   0, 0, 0, 0
        .byte   0, 0, 0, 0
        .byte   50
