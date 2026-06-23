        ; double to float32 (truncate mantissa)
        ; input double in DE:HL:DE':HL', output float32 in HL:DE
        ; (H=byte3 sign+exp, L=byte2, D=byte1, E=byte0)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module db2fs
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___db2fs

        ; ___db2fs
        ; inputs:  DE:HL:DE':HL' = double
        ; outputs: HL:DE = float32
        ; clobbers: af, bc, de, hl, ix, de', hl'
___db2fs:
        push    ix
        ld      ix, #0
        add     ix, sp
        ld      b, h
        ld      c, l
        ld      hl, #-8
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

        ; biased exp11 = ((a7&0x7F)<<4)|(a6>>4)
        ld      a, -1(ix)
        and     #0x7F
        ld      h, #0
        ld      l, a
        add     hl, hl
        add     hl, hl
        add     hl, hl
        add     hl, hl          ; (a7&0x7F)<<4
        ld      a, -2(ix)
        rlca
        rlca
        rlca
        rlca
        and     #0x0F
        or      l
        ld      l, a            ; HL = exp11

        ; exp8 = exp11 - 896 (= 1023 - 127)
        ld      a, l
        sub     a, #0x80        ; 896 = 0x380
        ld      l, a
        ld      a, h
        sbc     a, #0x03
        ld      h, a            ; HL = exp8 (signed)
        ; if exp8 <= 0 → underflow → return 0.0
        jp      c, .ret_zero
        ld      a, h
        or      l
        jp      z, .ret_zero
        ; if exp8 >= 255 → clamp; H must be 0 and L < 255
        ld      a, h
        or      a
        jp      nz, .clamp_max
        ld      a, l
        cp      #0xFF
        jp      nc, .clamp_max

        ; exp8 in L (1..254). sign = a7 bit7.
        ld      a, -1(ix)
        and     #0x80
        ld      b, a            ; B = sign

        ; byte3 = sign | (exp8 >> 1)
        ld      a, l
        srl     a               ; exp8 >> 1, carry = exp8&1
        or      b               ; | sign
        exx
        ld      h, a            ; float byte3 → H' temporarily
        exx
        ; remember exp8 LSB in carry — save it
        ld      a, l
        and     #0x01
        ld      c, a            ; C = exp8 & 1

        ; mant23 = {a6[3:0], a5, a4, a3[7:5]}
        ; byte2 = (exp8&1)<<7 | mant23[22:16] = (C<<7) | {a6[3:0],a5[7:5]}
        ld      a, -2(ix)       ; a6
        and     #0x0F
        rlca
        rlca
        rlca                    ; a6[3:0] << 3 → bits[6:3]
        ld      d, a
        ld      a, -3(ix)       ; a5
        rlca
        rlca
        rlca                    ; a5>>5 → low 3 bits (via rotate then mask)
        and     #0x07
        or      d               ; {a6[3:0],a5[7:5]} = mant23[22:16] (7 bits)
        ld      d, a
        ld      a, c
        rrca                    ; C(bit0) → bit7
        or      d               ; byte2
        ld      l, a            ; float byte2 → L

        ; byte1 = mant23[15:8] = {a5[4:0], a4[7:5]}
        ld      a, -3(ix)       ; a5
        and     #0x1F
        rlca
        rlca
        rlca                    ; a5[4:0] << 3
        ld      d, a
        ld      a, -4(ix)       ; a4
        rlca
        rlca
        rlca
        and     #0x07           ; a4[7:5]
        or      d
        ld      e, a            ; save byte1 in E temporarily

        ; byte0 = mant23[7:0] = {a4[4:0], a3[7:5]}
        ld      a, -4(ix)       ; a4
        and     #0x1F
        rlca
        rlca
        rlca                    ; a4[4:0] << 3
        ld      d, a
        ld      a, -5(ix)       ; a3
        rlca
        rlca
        rlca
        and     #0x07           ; a3[7:5]
        or      d               ; byte0
        ; assemble: float = HL:DE, H=byte3, L=byte2, D=byte1, E=byte0
        ld      d, e            ; D = byte1
        ld      e, a            ; E = byte0
        exx
        ld      a, h            ; A = byte3 (saved in H')
        exx
        ld      h, a            ; H = byte3
        ; L already = byte2
        ld      sp, ix
        pop     ix
        ret

.clamp_max:
        ; return largest finite float: 0x7F7FFFFF (or with sign)
        ld      a, -1(ix)
        and     #0x80
        or      #0x7F
        ld      h, a            ; byte3 = sign|0x7F
        ld      l, #0x7F
        ld      d, #0xFF
        ld      e, #0xFF
        ld      sp, ix
        pop     ix
        ret

.ret_zero:
        ld      hl, #0
        ld      de, #0
        ld      sp, ix
        pop     ix
        ret
