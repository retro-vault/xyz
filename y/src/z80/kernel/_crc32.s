        ; Compute the IEEE CRC-32 used by XPRG images.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _crc32
        .optsdcc -mz80 sdcccall(1)
        .globl  __crc32
        .globl  __crc32_reset
        .globl  __crc32_update
        .globl  __crc32_finish
        .area   _CODE

        ; input: HL = bytes, BC = length.
        ; output: DE:HL = IEEE CRC-32; preserves IX and IY.
__crc32::
        push    hl
        push    bc
        call    __crc32_reset
        pop     bc
        pop     hl
        call    __crc32_update
        jp      __crc32_finish
