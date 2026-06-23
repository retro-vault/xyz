        ; fixed8_8_div8.s
        ;
        ; Signed 8.8 fixed-point divide by exact integer 8.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_div8
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_div8

        .area   _CODE

        ; inputs:  HL = a
        ; outputs: DE = a / 8, truncated toward zero
_fixed8_8_div8::
        bit     7,h
        jr      z,.shift
        call    .neg_hl
        call    .shr_hl_1
        call    .shr_hl_1
        call    .shr_hl_1
        call    .neg_hl
        ex      de,hl
        ret
.shift:
        call    .shr_hl_1
        call    .shr_hl_1
        call    .shr_hl_1
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
