        ; Return the low 16 bits of the ZX Spectrum clock.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module clock
        .optsdcc -mz80 sdcccall(1)
        .globl  __clock
        .globl  _clock_ticks
        ; Five bytes exactly fill the gap following RST 10h.
        .area   _CODE
__clock::
        ld      de, (_clock_ticks)
        ret
