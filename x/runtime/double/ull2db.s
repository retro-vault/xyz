        ; unsigned 64-bit (and helpers for smaller) to double
        ;
        ; Provides the shared core __ull2db_core that converts a 64-bit
        ; unsigned magnitude (in an IX frame) plus a sign flag into an
        ; IEEE-754 double, returned in DE:HL:DE':HL'.
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module ull2db
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___ull2db
        .globl  ___sll2db
        .globl  __ull2db_core
        .globl  __db_zero

        ; ___ull2db
        ; inputs:  unsigned 64-bit in DE:HL:DE':HL' (stack arg per call64_1arg)
        ;          Actually arg arrives in registers (DE=lo..HL'=hi).
        ; outputs: DE:HL:DE':HL' = (double)value
        ; clobbers: af, bc, de, hl, ix, de', hl'
___ull2db:
        push    ix
        ld      ix, #0
        add     ix, sp
        ; reserve 9 bytes: m[0..7] at ix-8..ix-1, sign at ix-9
        ld      b, h
        ld      c, l
        ld      hl, #-9
        add     hl, sp
        ld      sp, hl
        ld      h, b
        ld      l, c
        ; store magnitude bytes m0..m7 (lsb..msb)
        ld      -8(ix), e
        ld      -7(ix), d
        ld      -6(ix), l
        ld      -5(ix), h
        exx
        ld      -4(ix), e
        ld      -3(ix), d
        ld      -2(ix), l
        ld      -1(ix), h
        exx
        xor     a
        ld      -9(ix), a       ; sign = 0 (positive)
        call    __ull2db_core
        ld      sp, ix
        pop     ix
        ret

        ; ___sll2db
        ; inputs:  signed 64-bit in DE:HL:DE':HL'
        ; outputs: DE:HL:DE':HL' = (double)value
___sll2db:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        ld      hl, #-9
        add     hl, sp
        ld      sp, hl
        ld      h, b
        ld      l, c
        ld      -8(ix), e
        ld      -7(ix), d
        ld      -6(ix), l
        ld      -5(ix), h
        exx
        ld      -4(ix), e
        ld      -3(ix), d
        ld      -2(ix), l
        ld      -1(ix), h
        exx
        ; sign = bit7 of m7 (ix-1)
        xor     a
        ld      -9(ix), a
        bit     7, -1(ix)
        jr      z, .sll_pos
        ld      a, #0x80
        ld      -9(ix), a
        ; negate m[0..7]
        xor     a
        sub     a, -8(ix)
        ld      -8(ix), a
        ld      a, #0
        sbc     a, -7(ix)
        ld      -7(ix), a
        ld      a, #0
        sbc     a, -6(ix)
        ld      -6(ix), a
        ld      a, #0
        sbc     a, -5(ix)
        ld      -5(ix), a
        ld      a, #0
        sbc     a, -4(ix)
        ld      -4(ix), a
        ld      a, #0
        sbc     a, -3(ix)
        ld      -3(ix), a
        ld      a, #0
        sbc     a, -2(ix)
        ld      -2(ix), a
        ld      a, #0
        sbc     a, -1(ix)
        ld      -1(ix), a
.sll_pos:
        call    __ull2db_core
        ld      sp, ix
        pop     ix
        ret

        ; __ull2db_core
        ; inputs:  magnitude m[0..7] at ix-8..ix-1 (lsb..msb), sign at ix-9
        ; outputs: DE:HL:DE':HL' = double
        ; frame: caller provides ix-9..ix-1; this routine may use ix-9..ix-1 only
        ; clobbers: af, bc, de, hl, de', hl'
__ull2db_core:
        ; check for zero
        ld      a, -8(ix)
        or      -7(ix)
        or      -6(ix)
        or      -5(ix)
        or      -4(ix)
        or      -3(ix)
        or      -2(ix)
        or      -1(ix)
        jr      nz, .nz
        jp      __db_zero

.nz:
        ; Find highest set bit index e (0..63).
        ; Strategy: shift magnitude LEFT until bit63 (m7 bit7) = 1,
        ; counting shifts in C. Then e = 63 - C.
        ld      c, #0
.findmsb:
        bit     7, -1(ix)
        jr      nz, .msb_found
        ; shift m[] left by 1
        sla     -8(ix)
        rl      -7(ix)
        rl      -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)
        rl      -2(ix)
        rl      -1(ix)
        inc     c
        jr      .findmsb
.msb_found:
        ; e = 63 - C. biased_exp = 1023 + e = 1086 - C.
        ; Now m7 bit7 = leading 1 (at bit position 63 of the m[] field).
        ; For a double, the leading 1 must be at bit 52. Currently it's at
        ; bit 63, so we must shift RIGHT by 11 to place it at bit 52.
        ; After that shift, bits [51:0] are the mantissa (bit52 = implicit 1).
        ;
        ; Shift m[] right by 11 bits. 11 = 8 + 3: drop the lowest byte then
        ; shift right 3 within the remaining, but we need rounding-free truncation.
        ; Simpler: shift right 11 times (each a full 64-bit shift). 11 iters.
        ld      b, #11
.shr11:
        srl     -1(ix)
        rr      -2(ix)
        rr      -3(ix)
        rr      -4(ix)
        rr      -5(ix)
        rr      -6(ix)
        rr      -7(ix)
        rr      -8(ix)
        djnz    .shr11

        ; Now m[] holds the 53-bit significand: bit52 = 1 (implicit),
        ; bits[51:0] = stored mantissa. m7 should be 0x00, m6 has bit4 set
        ; (bit52 = byte6 bit4).
        ;
        ; biased_exp = 1086 - C  (11-bit value, 0..2047)
        ; 1086 = 0x043E. Compute exp into HL = 0x043E - C.
        ld      hl, #0x043E
        ld      b, #0
        ld      a, l
        sub     c
        ld      l, a
        ld      a, h
        sbc     a, b
        ld      h, a                 ; HL = biased_exp (0..2047)

        ; Now insert exponent and sign into the top bytes.
        ; byte7 = sign | exp[10:4]
        ; byte6 = (exp[3:0] << 4) | mant[51:48]   (mant[51:48] currently in m6[3:0],
        ;          with bit4 of m6 = implicit 1 which we must clear)
        ;
        ; First clear implicit 1 (bit4 of m6 = ix-2)
        res     4, -2(ix)

        ; exp[10:4] = HL >> 4 (low 7 bits)
        ; exp[3:0]  = L & 0x0F
        ; Build byte7:
        ld      a, l
        ; we need (exp >> 4): HL has 11 bits. exp>>4 = ((H<<4)|(L>>4)) & 0x7F
        push    hl
        ld      a, l
        srl     a
        srl     a
        srl     a
        srl     a               ; A = L >> 4 (low nibble of exp[7:4])
        ld      b, a
        ld      a, h
        add     a, a
        add     a, a
        add     a, a
        add     a, a            ; A = H << 4
        or      b               ; A = (H<<4)|(L>>4) = exp[10:4]  (bit7=0 since exp<2048)
        and     #0x7F
        ; OR in sign
        ld      b, a
        ld      a, -9(ix)       ; sign (0x00 or 0x80)
        or      b               ; byte7
        exx
        ld      h, a            ; H' = byte7
        exx
        pop     hl

        ; byte6 = (exp[3:0] << 4) | mant[51:48]
        ld      a, l
        and     #0x0F
        rlca
        rlca
        rlca
        rlca                    ; A = exp[3:0] << 4
        ld      b, a
        ld      a, -2(ix)       ; m6 = mant[51:48] in low nibble (bit4 cleared)
        and     #0x0F
        or      b               ; byte6
        exx
        ld      l, a            ; L' = byte6
        exx

        ; byte5 = m5 (mant[47:40]) = ix-3
        ld      a, -3(ix)
        exx
        ld      d, a            ; D' = byte5
        exx
        ; byte4 = m4 (mant[39:32]) = ix-4
        ld      a, -4(ix)
        exx
        ld      e, a            ; E' = byte4
        exx
        ; byte3 = m3 = ix-5
        ld      h, -5(ix)
        ; byte2 = m2 = ix-6
        ld      l, -6(ix)
        ; byte1 = m1 = ix-7
        ld      d, -7(ix)
        ; byte0 = m0 = ix-8
        ld      e, -8(ix)
        ret
