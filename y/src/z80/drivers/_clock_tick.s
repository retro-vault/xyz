        ; Advance the 32-bit tick counter and wall clock at 50 Hz.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _clock_tick
        .optsdcc -mz80 sdcccall(1)
        .globl  __clock_tick
        .globl  _clock_ticks
        .globl  _clock_time
        .globl  _clock_sec_countdown
        .area   _CODE

__clock_tick::
        ld      hl, (_clock_ticks)
        inc     hl
        ld      (_clock_ticks), hl
        ld      a, h
        or      l
        jr      nz, .seconds
        ld      hl, (_clock_ticks+2)
        inc     hl
        ld      (_clock_ticks+2), hl
.seconds:
        ld      a, (_clock_sec_countdown)
        dec     a
        jr      z, .elapsed
        ld      (_clock_sec_countdown), a
        ret
.elapsed:
        ld      a, #50
        ld      (_clock_sec_countdown), a
        ld      hl, (_clock_time)
        inc     hl
        ld      (_clock_time), hl
        ld      a, h
        or      l
        ret     nz
        ld      hl, (_clock_time+2)
        inc     hl
        ld      (_clock_time+2), hl
        ret
