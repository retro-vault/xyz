        ; float -> signed long (ieee-754 single) for the runtime
        ; converts 32-bit float to 32-bit signed int with truncation
        ; toward zero.
        ; behavior:
        ;   |x| < 1              -> 0
        ;   x >=  2^31           ->  0x7FFFFFFF (clamp)
        ;   x <= -2^31           ->  0x80000000 (clamp)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fs2slong
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  ___fs2slong
        .globl  __fs2u32mag
        .globl  __fp_zero32

        ; ___fs2slong
        ; inputs:  DE:HL = IEEE-754 single
        ; outputs: HL:DE = signed 32-bit integer (trunc toward zero,
        ; saturating)
        ; clobbers: af, bc, de, hl
___fs2slong:
        ; arrange: HL = low word, DE = high word
        ex      de,hl

        ; sign flag in B (0x80 or 0)
        ld      a,d
        and     #0x80
        ld      b,a

        ; exp = ((D & 0x7F) << 1) | (E >> 7)
        ld      a,d
        and     #0x7F
        rlca
        ld      c,a
        bit     7,e
        jr      z, .exp_done
        inc     c
.exp_done:
        ; unbiased e
        ld      a,c
        sub     #127
        ld      c,a

        ; if e < 0 -> 0
        jr      nc, .e_nonneg
        call    __fp_zero32
        jr      .ret32

.e_nonneg:
        ; clamp if e >= 31
        ld      a,c
        cp      #31
        jr      c, .build_mag

        bit     7,b
        jr      z, .clamp_pos
        ld      d,#0x80
        ld      e,#0x00
        ld      h,#0x00
        ld      l,#0x00
        jr      .ret32
.clamp_pos:
        ld      d,#0x7F
        ld      e,#0xFF
        ld      h,#0xFF
        ld      l,#0xFF
        jr      .ret32

.build_mag:
        call    __fs2u32mag

        ; DE:HL is magnitude
        bit     7,b
        jr      z, .pos_path

        ; negative: clamp if magnitude > 0x80000000
        ld      a,d
        cp      #0x80
        jr      c, .neg_twos
        jr      nz, .neg_clamp
        ld      a,e
        or      h
        or      l
        jr      z, .ret32               ; exactly 0x80000000
.neg_clamp:
        ld      d,#0x80
        ld      e,#0x00
        ld      h,#0x00
        ld      l,#0x00
        jr      .ret32

.neg_twos:
        ; two's complement negate DE:HL
        ld      a,l
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        ld      a,e
        cpl
        ld      e,a
        ld      a,d
        cpl
        ld      d,a
        inc     hl
        jr      nz, .ret32
        inc     de
        jr      .ret32

.pos_path:
        ; positive: clamp if >= 0x80000000
        ld      a,d
        cp      #0x80
        jr      c, .ret32
        ld      d,#0x7F
        ld      e,#0xFF
        ld      h,#0xFF
        ld      l,#0xFF

.ret32:
        ; SDCC expects hl:de for 32-bit return
        ex      de,hl
        ret
