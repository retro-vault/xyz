        ; fixed24_8_to_int.s
        ;
        ; Convert 24.8 fixed to signed int by truncating toward zero.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_to_int
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_to_int

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE = signed int
_fixed24_8_to_int::
        bit     7,h
        jr      z,.positive
        call    .neg_dehl
        ld      e,d
        ld      d,l
        call    .neg_de
        ret
.positive:
        ld      e,d
        ld      d,l
        ret

.neg_dehl:
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ld      a,#0
        sbc     a,l
        ld      l,a
        ld      a,#0
        sbc     a,h
        ld      h,a
        ret

.neg_de:
        xor     a
        sub     a,e
        ld      e,a
        ld      a,#0
        sbc     a,d
        ld      d,a
        ret
