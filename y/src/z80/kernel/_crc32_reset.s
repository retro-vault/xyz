        ; Start one incremental IEEE CRC-32 calculation.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _crc32_reset
        .optsdcc -mz80 sdcccall(1)
        .globl  __crc32_reset
        .globl  __crc32_state
        .area   _ROM_AUX
__crc32_reset::
        ld      hl, #0xffff
        ld      (__crc32_state), hl
        ld      (__crc32_state+2), hl
        ret
