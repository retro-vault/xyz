        ; signed/unsigned int16 to double — widen to 64-bit, use core
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module sint2db
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___sint2db
        .globl  ___uint2db
        .globl  __ull2db_core

        ; ___sint2db — int16 in HL → double in DE:HL:DE':HL'
___sint2db:
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
        ; magnitude + sign
        xor     a
        ld      -9(ix), a       ; sign = 0
        bit     7, h
        jr      z, .spos
        ld      a, #0x80
        ld      -9(ix), a       ; sign = negative
        ; negate HL
        xor     a
        sub     a, l
        ld      l, a
        sbc     a, a
        sub     a, h
        ld      h, a
.spos:
        ; store magnitude: m0=L, m1=H, m2..m7 = 0
        ld      -8(ix), l
        ld      -7(ix), h
        xor     a
        ld      -6(ix), a
        ld      -5(ix), a
        ld      -4(ix), a
        ld      -3(ix), a
        ld      -2(ix), a
        ld      -1(ix), a
        call    __ull2db_core
        ld      sp, ix
        pop     ix
        ret

        ; ___uint2db — uint16 in HL → double
___uint2db:
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
        ld      -8(ix), l
        ld      -7(ix), h
        xor     a
        ld      -9(ix), a
        ld      -6(ix), a
        ld      -5(ix), a
        ld      -4(ix), a
        ld      -3(ix), a
        ld      -2(ix), a
        ld      -1(ix), a
        call    __ull2db_core
        ld      sp, ix
        pop     ix
        ret
