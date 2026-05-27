        ; unsigned int to float (ieee-754 single) for the runtime
        ; converts a 16-bit unsigned int (0..65535) to 32-bit single-
        ; precision float.
        ; exact; no fractional part to round. sign is always zero.
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module uint2fs
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE

        ; ___uint2fs
        ; inputs:  hl = a (unsigned 16-bit)
        ; outputs: hl:de = (float)a  (ieee-754 single, hl=high word,
        ; de=low word)
        ; clobbers: af, bc, de, hl
        .globl  ___uint2fs
        ; ___uint2fs
___uint2fs:
        ; zero?
        ld      a,h
        or      l
        jr      nz, .nonzero
        xor     a
        ld      h,a
        ld      l,a
        ld      d,a
        ld      e,a
        ret

.nonzero:
        ; normalize HL until bit15 is 1; count shifts in C
        ld      c,#0x00
.norm:
        bit     7,h
        jr      nz, .normdone
        add     hl,hl
        inc     c
        jr      .norm
.normdone:
        ; exponent = 127 + (15 - C) = 142 - C
        ld      a,#142
        sub     c                       ; A = exponent (8-bit)

        ; pack ieee:
        ; first output byte = exp >> 1   (sign=0)
        ; second output byte = ((exp&1)<<7) | (H & 0x7F)
        srl     a                       ; A = exp>>1, carry = exp&1
        ld      b,a                     ; B = first byte

        ld      a,h
        jr      c, .exp_odd
        and     #0x7F
.exp_odd:
        ld      c,a

        ; mantissa low 16 bits = (L << 8)
        ; low word bytes: [L][0]
        ld      d,l
        xor     a
        ld      e,a

        ; return float in HL:DE (high word in HL, low in DE)
        ld      h,b
        ld      l,c
        ret
