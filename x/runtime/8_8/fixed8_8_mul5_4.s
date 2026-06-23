        ; fixed8_8_mul5_4.s
        ;
        ; Signed 8.8 fixed-point multiply by exact 5/4.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_mul5_4
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_mul5_4

        .area   _CODE

        ; inputs:  HL = a
        ; outputs: DE = a * 5/4, truncated toward zero
_fixed8_8_mul5_4::
        ld      b,h
        ld      c,l
        bit     7,h
        jr      z,.shift
        call    .neg_hl
        call    .shr_hl_1
        call    .shr_hl_1
        call    .neg_hl
        jr      .add_original
.shift:
        call    .shr_hl_1
        call    .shr_hl_1
.add_original:
        add     hl,bc
        ex      de,hl
        ret

.shr_hl_1:
        srl     h
        rr      l
        ret

.neg_hl:
        xor     a
        sub     a,l
        ld      l,a
        ld      a,#0
        sbc     a,h
        ld      h,a
        ret
