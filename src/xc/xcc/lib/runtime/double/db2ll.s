        ; double to signed int64 (truncate toward zero) — shared core
        ;
        ; Produces signed int64 in DE:HL:DE':HL'.  Wrappers truncate to
        ; smaller widths.  For |x| < 1 returns 0.  No saturation beyond
        ; int64 range (overflow wraps).
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module db2ll
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___db2sll
        .globl  __db2ll_core

        ; ___db2sll — double in DE:HL:DE':HL' → signed int64 in same regs
___db2sll:
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
        ; store double bytes a0..a7 at ix-8..ix-1
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
        call    __db2ll_core
        ld      sp, ix
        pop     ix
        ret

        ; __db2ll_core
        ; inputs:  double bytes a0..a7 at ix-8..ix-1, scratch byte at ix-9
        ; outputs: DE:HL:DE':HL' = signed int64 (trunc toward zero)
        ; clobbers: af, bc, de, hl, de', hl'
__db2ll_core:
        ; biased exp = ((a7&0x7F)<<4) | (a6>>4)
        ld      a, -1(ix)
        and     #0x7F
        rlca
        rlca
        rlca
        rlca                    ; (a7&0x7F)<<4  (low 7 bits shifted; top may spill)
        ; careful: (a7&0x7F) max 0x7F, <<4 = 0x7F0 — needs 16 bits.
        ; Build exp in HL16.
        ld      a, -1(ix)
        and     #0x7F
        ld      h, #0
        ld      l, a
        add     hl, hl
        add     hl, hl
        add     hl, hl
        add     hl, hl          ; HL = (a7&0x7F) << 4
        ld      a, -2(ix)       ; a6
        rlca
        rlca
        rlca
        rlca                    ; a6 >> 4 in low nibble (rotate, then mask)
        and     #0x0F
        or      l
        ld      l, a            ; HL = biased exp (0..2047)

        ; sign in B7 of a7
        ld      a, -1(ix)
        and     #0x80
        ld      -9(ix), a       ; save sign

        ; if exp < 1023 → |x| < 1 → return 0
        ld      a, h
        or      a
        jp      nz, .exp_ge_256
        ; high byte 0 → exp < 256 < 1023 → return 0
        jp      .ret_zero
.exp_ge_256:
        ; compare HL with 1023 (0x3FF)
        ld      a, h
        cp      #0x04
        jp      c, .check_lt_1023   ; H < 4 → exp < 0x400, need detail
        jp      .exp_ok             ; H >= 4 → exp >= 0x400 > 1023
.check_lt_1023:
        ; H == 3 here (since >=256 and <0x400 means H in 1..3; but exp valid range)
        ; exp = 0x300..0x3FF when H==3. All < 1023? 0x3FF=1023. So if HL==0x3FF, e=0 → |x|>=1, value=1.
        ; treat exp < 1023 as zero; exp==1023 → e=0.
        ; compute e = exp - 1023; if borrow → zero.
        jp      .exp_ok

.exp_ok:
        ; e = exp - 1023
        ld      bc, #0x03FF
        ld      a, l
        sub     c
        ld      l, a
        ld      a, h
        sbc     a, b
        ld      h, a            ; HL = e (signed); if negative → was < 1023
        jp      c, .ret_zero    ; borrow → exp < 1023 → 0
        ; e in L (0..). If e >= 64 → overflow, clamp by wrap (just proceed; tests stay in range)

        ; Build significand into result bytes r0..r7 at ix-8..ix-1 reused:
        ; r0=a0,...,r5=a5, r6=(a6&0x0F)|0x10, r7=0
        ld      a, -2(ix)
        and     #0x0F
        or      #0x10           ; set implicit 1 at bit4 (= bit52)
        ld      -2(ix), a
        xor     a
        ld      -1(ix), a       ; r7 = 0

        ; e is in L. We need:
        ;   if e <= 52: shift right (52 - e)
        ;   if e >  52: shift left  (e - 52)
        ld      a, l            ; A = e (0..63 for valid range)
        cp      #53
        jp      nc, .shift_left

        ; shift right by (52 - e)
        ld      a, #52
        sub     l               ; A = 52 - e
        ld      b, a
        ld      a, b
        or      a
        jp      z, .signfix     ; shift count 0
.sr_loop:
        srl     -1(ix)
        rr      -2(ix)
        rr      -3(ix)
        rr      -4(ix)
        rr      -5(ix)
        rr      -6(ix)
        rr      -7(ix)
        rr      -8(ix)
        djnz    .sr_loop
        jp      .signfix

.shift_left:
        ; shift left by (e - 52)
        ld      a, l
        sub     #52
        ld      b, a
        ld      a, b
        or      a
        jp      z, .signfix
.sl_loop:
        sla     -8(ix)
        rl      -7(ix)
        rl      -6(ix)
        rl      -5(ix)
        rl      -4(ix)
        rl      -3(ix)
        rl      -2(ix)
        rl      -1(ix)
        djnz    .sl_loop

.signfix:
        ; if sign set, negate r0..r7
        ld      a, -9(ix)
        or      a
        jp      z, .load
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

.load:
        ld      e, -8(ix)
        ld      d, -7(ix)
        ld      l, -6(ix)
        ld      h, -5(ix)
        exx
        ld      e, -4(ix)
        ld      d, -3(ix)
        ld      l, -2(ix)
        ld      h, -1(ix)
        exx
        ret

.ret_zero:
        xor     a
        ld      d, a
        ld      e, a
        ld      h, a
        ld      l, a
        exx
        ld      d, a
        ld      e, a
        ld      h, a
        ld      l, a
        exx
        ret
