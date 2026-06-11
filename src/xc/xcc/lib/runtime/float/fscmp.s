        ; float compare (ieee-754 single) for the runtime
        ; returns -1 if a<b, 0 if a==b, +1 if a>b
        ; denormals treated as 0; nan/inf unsupported.
        ;
        ; abi (observed):
        ;   a in regs: hl:de (h=a3, l=a2, d=a1, e=a0)
        ;   b on stack: ret, b.low, b.high (caller pushes b.high then
        ; b.low)
        ;   return int16 in de (caller does ex de,hl)
        ;
        ; gpl-2.0-or-later (see: LICENSE)
        ; copyright (c) 2025 tomaz stih

        .module fscmp
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE                   ; code segment

        .globl  ___fscmp                ; export symbols
        .globl  __fp_retpop4

        ; ___fscmp
        ; inputs:  hl:de = a (float), stack = b (float)
        ; outputs: de = -1, 0, +1
        ; clobbers: a, f, b, c, d, e, h, l, ix
___fscmp::
        push    ix
        ld      ix, #0
        add     ix, sp

        ; snapshot a into b,c,d,e so we never lose a1/a0
        ; b=a3, c=a2, d=a1, e=a0
        ld      b, h
        ld      c, l

        ; stack layout after push ix:
        ;   2(ix),3(ix) = return address
        ;   4(ix),5(ix) = b.low  (b0=4(ix), b1=5(ix))
        ;   6(ix),7(ix) = b.high (b2=6(ix), b3=7(ix))

        ; compute biased exponents: ea in h, eb in l
        ; ea = ((a3&0x7f)<<1) | (a2>>7)
        ld      a, b
        and     #0x7f
        rla
        ld      h, a
        ld      a, c
        and     #0x80
        jr      z, .ea_ok
        inc     h
.ea_ok:
        ; eb = ((b3&0x7f)<<1) | (b2>>7) (b3=7(ix), b2=6(ix))
        ld      a, 7(ix)
        and     #0x7f
        rla
        ld      l, a
        ld      a, 6(ix)
        and     #0x80
        jr      z, .eb_ok
        inc     l
.eb_ok:

        ; NaNs are unordered. Return a dedicated sentinel so equality is false
        ; and higher layers can map all relational operators correctly.
        ld      a, h
        cp      #0xff
        jr      nz, .a_not_nan
        ld      a, c
        and     #0x7f
        or      d
        or      e
        jp      nz, .retu
.a_not_nan:
        ld      a, l
        cp      #0xff
        jr      nz, .b_not_nan
        ld      a, 6(ix)
        and     #0x7f
        or      5(ix)
        or      4(ix)
        jp      nz, .retu
.b_not_nan:

        ; denormals treated as 0 (exp==0)
        ld      a, h
        or      a
        jr      nz, .a_nz
        ld      a, l
        or      a
        jr      nz, .a_zero_b_nz
        jp      .ret0

.a_zero_b_nz:
        ; 0 vs nonzero: sign(b) decides (b3=7(ix))
        ld      a, 7(ix)
        and     #0x80
        jr      z, .retm1
        jr      .retp1

.a_nz:
        ld      a, l
        or      a
        jr      nz, .both_nz
        ; a!=0, b==0: sign(a) decides
        ld      a, b
        and     #0x80
        jr      z, .retp1
        jr      .retm1

.both_nz:
        ; signs differ?
        ; (a3 ^ b3) & 0x80
        ld      a, b
        xor     7(ix)
        and     #0x80
        jr      z, .same_sign
        ; different signs: positive is greater
        ld      a, b
        and     #0x80
        jr      z, .retp1
        jr      .retm1

.same_sign:
        ; compare exponents (biased): h vs l
        ld      a, h
        cp      l
        jr      z, .exp_equal

        ; if negative: reversed
        bit     7, b
        jr      z, .pos_exp

        ; both negative
        ld      a, h
        cp      l
        jr      c, .retp1
        jr      .retm1

.pos_exp:
        ld      a, h
        cp      l
        jr      c, .retm1
        jr      .retp1

.exp_equal:
        ; same sign, same exponent:
        ; compare a2:a1:a0 vs b2:b1:b0
        ; (safe because exponent equal => bit7 of a2/b2 equal too)
        ;
        ; important: preserve carry from cp -> use bit for sign test

        ; a2 vs b2
        ld      a, c
        cp      6(ix)
        jr      z, .mant_mid

        bit     7, b
        jr      z, .mant_pos_a2
        ; negative reversed
        jr      c, .retp1
        jr      .retm1
.mant_pos_a2:
        jr      c, .retm1
        jr      .retp1

.mant_mid:
        ; a1 vs b1
        ld      a, d
        cp      5(ix)
        jr      z, .mant_low

        bit     7, b
        jr      z, .mant_pos_a1
        ; negative reversed
        jr      c, .retp1
        jr      .retm1
.mant_pos_a1:
        jr      c, .retm1
        jr      .retp1

.mant_low:
        ; a0 vs b0
        ld      a, e
        cp      4(ix)
        jr      z, .ret0

        bit     7, b
        jr      z, .mant_pos_a0
        ; negative reversed
        jr      c, .retp1
        jr      .retm1
.mant_pos_a0:
        jr      c, .retm1
        jr      .retp1

.retm1:
        ld      de, #0xffff
        jr      .ret

.ret0:
        ld      de, #0x0000
        jr      .ret

.retu:
        ld      de, #0x8000
        jr      .ret

.retp1:
        ld      de, #0x0001
        ; fallthrough

.ret:
        pop     ix
        jp      __fp_retpop4
