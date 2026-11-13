        ; Finish one incremental IEEE CRC-32 calculation.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _crc32_finish
        .optsdcc -mz80 sdcccall(1)
        .globl  __crc32_finish
        .globl  __crc32_state
        .area   _CODE

        ; output: DE:HL = complemented IEEE CRC-32; preserves IX and IY.
__crc32_finish::
        ld      hl, (__crc32_state)
        ld      de, (__crc32_state+2)
        ld      a, d
        cpl
        ld      d, a
        ld      a, e
        cpl
        ld      e, a
        ld      a, h
        cpl
        ld      h, a
        ld      a, l
        cpl
        ld      l, a
        ret
