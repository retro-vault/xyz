        ; Serialized XPRG loader CRC accumulator.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _crc32_state
        .optsdcc -mz80 sdcccall(1)
        .globl  __crc32_state
        .area   _BSS
__crc32_state::
        .ds     4
