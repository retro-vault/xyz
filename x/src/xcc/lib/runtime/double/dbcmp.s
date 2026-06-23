        ; double compare — returns -1/0/+1 in DE
        ; a in DE:HL:DE':HL', b at ix+4..ix+11 (lsb..msb)
        ;
        ; For IEEE-754, comparing the magnitude bit-pattern as an unsigned
        ; integer yields the correct ordering for same-sign operands.
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module dbcmp
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___dbcmp

        ; ___dbcmp
        ; inputs:  a in DE:HL:DE':HL', b at ix+4..ix+11
        ; outputs: DE = -1 if a<b, 0 if a==b, +1 if a>b
        ; clobbers: af, bc, de, hl, ix, de', hl'
___dbcmp:
        push    ix
        ld      ix, #0
        add     ix, sp
        ; frame: a bytes at ix-8..ix-1 (a0..a7)
        ld      b, h
        ld      c, l
        ld      hl, #-8
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

	        ; NaNs are unordered. Return a dedicated sentinel so equality is
	        ; false and higher layers can map all relational operators
	        ; correctly.
	        ld      a, -1(ix)
	        and     #0x7F
	        cp      #0x7F
	        jr      nz, .a_not_nan
	        ld      a, -2(ix)
	        and     #0xF0
	        cp      #0xF0
	        jr      nz, .a_not_nan
	        ld      a, -2(ix)
	        and     #0x0F
	        or      -3(ix)
	        or      -4(ix)
	        or      -5(ix)
	        or      -6(ix)
	        or      -7(ix)
	        or      -8(ix)
	        jp      nz, .ret_u
.a_not_nan:
	        ld      a, 11(ix)
	        and     #0x7F
	        cp      #0x7F
	        jr      nz, .b_not_nan
	        ld      a, 10(ix)
	        and     #0xF0
	        cp      #0xF0
	        jr      nz, .b_not_nan
	        ld      a, 10(ix)
	        and     #0x0F
	        or      9(ix)
	        or      8(ix)
	        or      7(ix)
	        or      6(ix)
	        or      5(ix)
	        or      4(ix)
	        jp      nz, .ret_u
.b_not_nan:

	        ; sign_a = bit7 of a7 (ix-1), sign_b = bit7 of b7 (ix+11)
	        ; magnitude_a == 0 ?  (mask sign bit of a7)
        ld      a, -1(ix)
        and     #0x7F
        or      -2(ix)
        or      -3(ix)
        or      -4(ix)
        or      -5(ix)
        or      -6(ix)
        or      -7(ix)
        or      -8(ix)
        ; A==0 means a is zero
        ld      b, a            ; B = nonzero-flag for a (0 = a is zero)
        ld      a, 11(ix)
        and     #0x7F
        or      10(ix)
        or      9(ix)
        or      8(ix)
        or      7(ix)
        or      6(ix)
        or      5(ix)
        or      4(ix)
        ld      c, a            ; C = nonzero-flag for b (0 = b is zero)

        ; both zero?
        ld      a, b
        or      c
        jp      nz, .not_both_zero
        jp      .ret0
.not_both_zero:
        ; a zero?
        ld      a, b
        or      a
        jp      nz, .a_nonzero
        ; a is zero, b nonzero. result = sign_b ? +1 : -1
        bit     7, 11(ix)
        jp      nz, .ret_p1
        jp      .ret_m1
.a_nonzero:
        ld      a, c
        or      a
        jp      nz, .both_nonzero
        ; b is zero, a nonzero. result = sign_a ? -1 : +1
        bit     7, -1(ix)
        jp      nz, .ret_m1
        jp      .ret_p1
.both_nonzero:
        ; compare signs
        ld      a, -1(ix)
        xor     11(ix)
        bit     7, a
        jp      z, .same_sign
        ; signs differ: sign_a ? a<b(-1) : a>b(+1)
        bit     7, -1(ix)
        jp      nz, .ret_m1
        jp      .ret_p1

.same_sign:
        ; compare magnitude (a7..a0 vs b7..b0), sign bit masked on top byte.
        ; cmp from MSB down.
        ld      a, -1(ix)
        and     #0x7F
        ld      b, a            ; B = a7 masked
        ld      a, 11(ix)
        and     #0x7F
        ld      c, a            ; C = b7 masked
        ld      a, b
        cp      c
        jp      c, .mag_a_lt    ; a7 < b7
        jp      nz, .mag_a_gt   ; a7 > b7
        ; a7 == b7, compare a6 vs b6
        ld      a, -2(ix)
        cp      10(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ld      a, -3(ix)
        cp      9(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ld      a, -4(ix)
        cp      8(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ld      a, -5(ix)
        cp      7(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ld      a, -6(ix)
        cp      6(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ld      a, -7(ix)
        cp      5(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ld      a, -8(ix)
        cp      4(ix)
        jp      c, .mag_a_lt
        jp      nz, .mag_a_gt
        ; magnitudes equal → equal
        jp      .ret0

.mag_a_gt:
        ; |a| > |b|. If both negative, a < b. Else a > b.
        bit     7, -1(ix)
        jp      nz, .ret_m1
        jp      .ret_p1
.mag_a_lt:
        ; |a| < |b|. If both negative, a > b. Else a < b.
        bit     7, -1(ix)
        jp      nz, .ret_p1
        jp      .ret_m1

.ret_m1:
        ld      de, #0xFFFF     ; -1
        jp      .done
.ret0:
        ld      de, #0
        jp      .done
.ret_p1:
        ld      de, #1
        jp      .done
.ret_u:
        ld      de, #0x8000
        jp      .done
.done:
        ld      sp, ix
        pop     ix
        ret
