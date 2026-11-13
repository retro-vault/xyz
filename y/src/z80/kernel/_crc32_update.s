        ; Add one byte span to the serialized IEEE CRC-32 calculation.
        ; MIT License (see: LICENSE), Copyright (C) 2026 tomaz stih
        .module _crc32_update
        .optsdcc -mz80 sdcccall(1)
        .globl  __crc32_update
        .globl  __crc32_state
        .area   _CODE

        ; input: HL = bytes, BC = length; preserves IX and IY.
__crc32_update::
        push    ix
        push    hl
        pop     ix
        ld      hl, (__crc32_state)
        ld      de, (__crc32_state+2)
.byte:
        ld      a, b
        or      c
        jr      z, .done
        ld      a, l
        xor     0(ix)
        ld      l, a
        inc     ix
        push    bc
        ld      b, #8
.bit:
        srl     d
        rr      e
        rr      h
        rr      l
        jr      nc, .next_bit
        ld      a, d
        xor     #0xed
        ld      d, a
        ld      a, e
        xor     #0xb8
        ld      e, a
        ld      a, h
        xor     #0x83
        ld      h, a
        ld      a, l
        xor     #0x20
        ld      l, a
.next_bit:
        djnz    .bit
        pop     bc
        dec     bc
        jr      .byte
.done:
        ld      (__crc32_state), hl
        ld      (__crc32_state+2), de
        pop     ix
        ret
