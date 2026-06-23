        ; signed/unsigned int32 to double — widen to 64-bit, use core
        ; input: DE = low16, HL = high16 (existing 32-bit ABI)
        ;
        ; MIT License (see: LICENSE)
        ; copyright (C) 2026 tomaz stih

        .module slong2db
        .optsdcc -mz80 sdcccall(1)
        .area   _CODE
        .globl  ___slong2db
        .globl  ___ulong2db
        .globl  __ull2db_core

        ; ___slong2db — int32 in DE:HL → double
___slong2db:
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
        ; store m0=E,m1=D,m2=L,m3=H, m4..m7=0
        ld      -8(ix), e
        ld      -7(ix), d
        ld      -6(ix), l
        ld      -5(ix), h
        xor     a
        ld      -4(ix), a
        ld      -3(ix), a
        ld      -2(ix), a
        ld      -1(ix), a
        ld      -9(ix), a       ; sign = 0
        ; if negative (bit7 of H = m3), negate 32-bit and set sign
        bit     7, -5(ix)
        jr      z, .lpos
        ld      a, #0x80
        ld      -9(ix), a
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
        ; m4..m7 remain 0 (magnitude fits in 32 bits except INT32_MIN
        ; whose magnitude 0x80000000 still fits in 32 unsigned bits)
.lpos:
        call    __ull2db_core
        ld      sp, ix
        pop     ix
        ret

        ; ___ulong2db — uint32 in DE:HL → double
___ulong2db:
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
        xor     a
        ld      -4(ix), a
        ld      -3(ix), a
        ld      -2(ix), a
        ld      -1(ix), a
        ld      -9(ix), a
        call    __ull2db_core
        ld      sp, ix
        pop     ix
        ret
