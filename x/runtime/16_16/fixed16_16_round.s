        ; fixed16_16_round.s
        ;
        ; Round signed 16.16 to nearest integer, halfway away from zero.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_round
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_round
        .globl  _fixed16_16_trunc

        .area   _CODE

        ; inputs:  DE:HL = fixed16_16
        ; outputs: DE:HL = round(x)
_fixed16_16_round::
        bit     7,h
        jr      nz,.negative
        ld      a,d
        add     a,#0x80
        ld      d,a
        ld      a,l
        adc     a,#0
        ld      l,a
        ld      a,h
        adc     a,#0
        ld      h,a
        jp      _fixed16_16_trunc
.negative:
        ld      a,d
        sub     a,#0x80
        ld      d,a
        ld      a,l
        sbc     a,#0
        ld      l,a
        ld      a,h
        sbc     a,#0
        ld      h,a
        jp      _fixed16_16_trunc
