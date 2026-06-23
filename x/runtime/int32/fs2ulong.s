        ; float -> unsigned long (ieee-754 single) for the runtime
        ; converts 32-bit float to 32-bit unsigned int with truncation
        ; toward zero.
        ; behavior:
        ;   negative  -> 0
        ;   |x| < 1   -> 0
        ;   x >= 2^32 -> 0xFFFFFFFF (clamp)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fs2ulong
        .optsdcc -mz80 sdcccall(1)

        .area   _CODE
        .globl  ___fs2ulong
        .globl  __fs2u32mag
        .globl  __fp_zero32

        ; ___fs2ulong
        ; inputs:  DE:HL = IEEE-754 single
        ; outputs: HL:DE = unsigned 32-bit integer (trunc toward zero,
        ; saturating)
        ; clobbers: af, bc, de, hl
___fs2ulong:
        ; arrange: HL = low word, DE = high word
        ex      de,hl

        ; negative -> 0
        bit     7,d
        jr      z, .nonneg
        call    __fp_zero32
        jr      .ret32

.nonneg:
        ; exp = ((D & 0x7F) << 1) | (E >> 7)
        ld      a,d
        and     #0x7F
        rlca
        ld      c,a
        bit     7,e
        jr      z, .exp_done
        inc     c
.exp_done:
        ; unbiased e = exp - 127
        ld      a,c
        sub     #127
        ld      c,a

        ; if e < 0 -> 0
        jr      nc, .e_nonneg
        call    __fp_zero32
        jr      .ret32

.e_nonneg:
        ; clamp if e >= 32
        ld      a,c
        cp      #32
        jr      c, .build_mag
        ld      d,#0xFF
        ld      e,#0xFF
        ld      h,#0xFF
        ld      l,#0xFF
        jr      .ret32

.build_mag:
        call    __fs2u32mag

.ret32:
        ; SDCC expects hl:de for 32-bit return
        ex      de,hl
        ret
