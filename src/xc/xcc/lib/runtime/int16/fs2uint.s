        ; float -> unsigned int (ieee-754 single) for the runtime
        ; converts 32-bit float to 16-bit unsigned int with truncation
        ; toward zero.
        ; behavior:
        ;   negative -> 0
        ;   |x| < 1  -> 0
        ;   x >= 65536 -> 65535
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fs2uint
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  ___fs2uint
        .globl  __fs2u16mag

        ; ___fs2uint
        ; inputs:  DE:HL = IEEE-754 single
        ; outputs: DE = unsigned 16-bit integer (trunc toward zero,
        ; saturating)
        ; clobbers: af, bc, de, hl
___fs2uint:
        ; normalize to:
        ;   B:C = high word bytes (a3:a2)
        ;   H:L = low word bytes  (a1:a0)
        ex      de,hl
        ld      b,d
        ld      c,e

        ; negative -> 0
        bit     7,b
        jr      z,.nonneg
        xor     a
        ld      d,a
        ld      e,a
        ret

.nonneg:
        ; exponent field exp = ((B & 0x7F) << 1) | (C >> 7)
        ld      a,b
        and     #0x7F
        rlca
        ld      e,a
        bit     7,c
        jr      z,.exp_done
        inc     e
.exp_done:
        ; unbiased e = exp - 127
        ld      a,e
        sub     #127
        ld      e,a

        ; if e < 0 -> 0
        jr      nc,.e_nonneg
        xor     a
        ld      d,a
        ld      e,a
        ret

.e_nonneg:
        ; if e >= 16 -> clamp to 0xFFFF
        ld      a,e
        cp      #16
        jr      c,.within
        ld      de,#0xFFFF
        ret

.within:
        ; e in 0..15
        ld      a,e
        jp      __fs2u16mag
