        ; fixed24_8_round.s
        ;
        ; Round signed 24.8 to nearest integer, halfway away from zero.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed24_8_round
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed24_8_round
        .globl  _fixed24_8_trunc

        .area   _CODE

        ; inputs:  DE:HL = fixed24_8
        ; outputs: DE:HL = round(x)
_fixed24_8_round::
        bit     7,h
        jr      nz,.negative
        ld      a,e
        add     a,#0x80
        ld      e,a
        ld      a,d
        adc     a,#0
        ld      d,a
        ld      a,l
        adc     a,#0
        ld      l,a
        ld      a,h
        adc     a,#0
        ld      h,a
        jp      _fixed24_8_trunc
.negative:
        ld      a,e
        sub     a,#0x80
        ld      e,a
        ld      a,d
        sbc     a,#0
        ld      d,a
        ld      a,l
        sbc     a,#0
        ld      l,a
        ld      a,h
        sbc     a,#0
        ld      h,a
        jp      _fixed24_8_trunc
