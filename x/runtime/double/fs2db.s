        ; float32 to double
        ; input: float in HL:DE (H=byte3 sign+exp, L=byte2, D=byte1, E=byte0)
        ; output: double in DE:HL:DE':HL'
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module fs2db
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___fs2db
        .globl  __db_zero

        ; ___fs2db
        ; inputs:  HL:DE = float32 (H=a3,L=a2,D=a1,E=a0)
        ; outputs: DE:HL:DE':HL' = (double)value
        ; clobbers: af, bc, de, hl, ix, de', hl'
___fs2db:
        push    ix
        ld      ix, #0
        add     ix, sp
        ; save float bytes to a small frame: a0..a3 at ix-4..ix-1
        ld      b, h
        ld      c, l
        ld      hl, #-4
        add     hl, sp
        ld      sp, hl
        ld      h, b
        ld      l, c
        ld      -4(ix), e       ; a0
        ld      -3(ix), d       ; a1
        ld      -2(ix), l       ; a2
        ld      -1(ix), h       ; a3

        ; exponent field of float: e8 = ((a3&0x7F)<<1)|(a2>>7)
        ld      a, -1(ix)
        and     #0x7F
        rlca
        ld      b, a
        bit     7, -2(ix)
        jr      z, .e8ok
        inc     b
.e8ok:
        ; b = e8 (0..255). If 0 → denormal/zero → return 0.0
        ld      a, b
        or      a
        jr      nz, .nonzero
        call    __db_zero
        ld      sp, ix
        pop     ix
        ret

.nonzero:
        ; double biased exp = e8 + 896 (since 1023-127=896)
        ; compute into HL16
        ld      l, b
        ld      h, #0
        ld      bc, #896
        add     hl, bc          ; HL = double biased exp (0..1151)

        ; byte7 = sign | exp[10:4]
        ld      a, -1(ix)
        and     #0x80           ; sign
        ld      c, a            ; save sign
        ; exp[10:4] = HL >> 4
        ld      a, l
        srl     a
        srl     a
        srl     a
        srl     a
        ld      b, a            ; L>>4
        ld      a, h
        add     a, a
        add     a, a
        add     a, a
        add     a, a            ; H<<4
        or      b               ; exp[10:4]
        and     #0x7F
        or      c               ; | sign
        exx
        ld      h, a            ; H' = byte7
        exx

        ; byte6 = (exp[3:0]<<4) | mant[51:48]
        ; float mantissa top: a2 bits[6:0] = mant[22:16]; mant[51:48] are the
        ; top 4 of the 23-bit float mantissa = a2[6:3]
        ld      a, l
        and     #0x0F
        rlca
        rlca
        rlca
        rlca                    ; exp[3:0]<<4
        ld      b, a
        ld      a, -2(ix)       ; a2
        and     #0x7F           ; drop the exp bit (bit7)
        srl     a
        srl     a
        srl     a               ; a2[6:3] -> bits[3:0]  (mant[51:48])
        or      b
        exx
        ld      l, a            ; L' = byte6
        exx

        ; byte5 = mant[47:40] = {a2[2:0], a1[7:5]}
        ld      a, -2(ix)       ; a2
        and     #0x07           ; a2[2:0]
        rlca
        rlca
        rlca
        rlca
        rlca                    ; <<5 -> bits[7:5]
        ld      b, a
        ld      a, -3(ix)       ; a1
        srl     a
        srl     a
        srl     a               ; a1[7:5] -> bits[4:0]... need top3 in [4:2]?
        ; mant[47:40]: a2[2:0] gives mant[47:45], a1[7:5] gives mant[44:42]?
        ; Actually float mant23 maps to double mant[51:29]. So:
        ; double mant[51:29] = float mant[22:0]; double mant[28:0] = 0.
        ; mant[51:48]=fm[22:19], mant[47:40]=fm[18:11], mant[39:32]=fm[10:3],
        ; mant[31:29]=fm[2:0], rest 0.
        ; fm[22:16]=a2[6:0], fm[15:8]=a1, fm[7:0]=a0.
        ; mant[47:40]=fm[18:11]={a2[2:0],a1[7:3]}
        ; redo: a2[2:0]<<5 | a1[7:3]
        ; a1[7:3] = a1 >> 3
        or      b
        exx
        ld      d, a            ; D' = byte5
        exx

        ; byte4 = mant[39:32] = fm[10:3] = {a1[2:0], a0[7:3]}
        ld      a, -3(ix)       ; a1
        and     #0x07
        rlca
        rlca
        rlca
        rlca
        rlca                    ; a1[2:0]<<5
        ld      b, a
        ld      a, -4(ix)       ; a0
        srl     a
        srl     a
        srl     a               ; a0>>3 = a0[7:3]
        or      b
        exx
        ld      e, a            ; E' = byte4
        exx

        ; byte3 = mant[31:24] = fm[2:0]<<5 = {a0[2:0],00000}
        ld      a, -4(ix)       ; a0
        and     #0x07
        rlca
        rlca
        rlca
        rlca
        rlca                    ; <<5
        ld      h, a            ; byte3
        ; byte2,1,0 = 0
        ld      l, #0
        ld      d, #0
        ld      e, #0
        ld      sp, ix
        pop     ix
        ret
