        ; float -> signed int (ieee-754 single) for the runtime
        ; converts 32-bit float to 16-bit signed int with truncation
        ; toward zero.
        ;
        ; behavior:
        ;   |x| < 1        -> 0
        ;   x >=  32768    ->  32767
        ;   x <= -32768    -> -32768
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fs2sint
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  ___fs2sint
        .globl  __fs2u16mag

        ; ___fs2sint
        ; inputs:  DE:HL = IEEE-754 single (E=a0, D=a1, L=a2, H=a3)
        ; outputs: DE = signed 16-bit integer (trunc toward zero,
        ; saturating)
        ; clobbers: af, bc, de, hl
___fs2sint:
        ; normalize to:
        ;   B:C = high word bytes (a3:a2)
        ;   H:L = low word bytes  (a1:a0)
        ex      de,hl
        ld      b,d
        ld      c,e

        ; exponent field exp = ((B & 0x7F) << 1) | (C >> 7)
        ld      a,b
        and     #0x7F
        rlca
        ld      e,a
        bit     7,c
        jr      z,.exp_done
        inc     e
.exp_done:
        ; unbiased exponent in E
        ld      a,e
        sub     #127
        ld      e,a

        ; |x| < 1 -> 0
        jr      nc,.e_nonneg
        xor     a
        ld      d,a
        ld      e,a
        ret

.e_nonneg:
        ; split by sign
        bit     7,b
        jr      z,.pos

        ; negative path: clamp for e >= 15
        ld      a,e
        cp      #15
        jr      c,.neg_within
        ld      de,#0x8000
        ret

.neg_within:
        ; e in 0..14 -> magnitude helper, then negate to signed result
        ld      a,e
        call    __fs2u16mag
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ret

.pos:
        ; positive path: clamp for e >= 15
        ld      a,e
        cp      #15
        jr      c,.pos_within
        ld      de,#0x7FFF
        ret

.pos_within:
        ; e in 0..14
        ld      a,e
        jp      __fs2u16mag
