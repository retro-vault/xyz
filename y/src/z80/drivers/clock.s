        ; Return the low 16 bits of the ZX Spectrum clock.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module clock
        .optsdcc -mz80 sdcccall(1)
        .globl  __clock
        .globl  _clock_ticks
        ; Pack this five-byte accessor into a fixed ROM gap.
        .if YOS_UNIFIED_48K
        .area _ROM_CLOCK
        .else
        .area _ROM_TINY
        .endif
__clock::
        ld      de, (_clock_ticks)
        ret
