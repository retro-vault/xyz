        ; unsigned long to float (ieee-754 single) for the runtime
        ; converts a 32-bit unsigned long (0..4294967295) to 32-bit
        ; single-precision float with rounding-to-nearest-even.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module ulong2fs
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE                   ; code segment

        .globl  ___ulong2fs             ; export symbols

        ; ___ulong2fs
        ; inputs:  hl:de = a (unsigned 32-bit, hl low, de high)
        ; outputs: hl:de = (float)a (ieee-754 single, hl=high, de=low)
        ; clobbers: a, b, c, d, e, h, l, f
___ulong2fs::
        ex      de, hl                  ; hl = low, de = high
        ld      b, d
        ld      c, e                    ; bc = high word

        ; zero?
        ld      a, b
        or      c
        or      h
        or      l
        jr      nz, .nonzero
        xor     a
        ld      h, a
        ld      l, a
        ld      d, a
        ld      e, a
        ret

.nonzero:
        ld      e, #0x00                ; shift count
.norm:
        bit     7, b
        jr      nz, .norm_done
        add     hl, hl
        rl      c
        rl      b
        inc     e
        jr      .norm
.norm_done:
        ld      a, #158
        sub     e
        ld      d, a                    ; d = exponent

        ; rounding uses discarded byte l; kept bytes are b:c:h
        ld      a, l
        cp      #0x80
        jr      c, .rounded
        jr      nz, .round_up
        ld      a, h
        and     #0x01
        jr      z, .rounded
.round_up:
        inc     h
        jr      nz, .rounded
        inc     c
        jr      nz, .rounded
        inc     b
        jr      nz, .rounded
        ld      b, #0x80
        xor     a
        ld      c, a
        ld      h, a
        inc     d

.rounded:
        ; save byte3 (mantissa low byte) before packing overwrites regs
        ld      e, h                    ; e = byte3

        ; pack exponent into byte0/byte1 (sign=0)
        ld      a, d
        srl     a
        ld      h, a                    ; h = byte0

        ld      a, b
        and     #0x7f
        bit     0, d
        jr      z, .no_explsb
        or      #0x80
.no_explsb:
        ld      l, a                    ; l = byte1

        ; low word bytes
        ld      d, c                    ; d = byte2
        ; e already = byte3
        ret
