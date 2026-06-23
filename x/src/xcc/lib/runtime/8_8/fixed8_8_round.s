        ; fixed8_8_round.s
        ;
        ; Round signed 8.8 to nearest integer, halfway away from zero.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed8_8_round
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed8_8_round
        .globl  _fixed8_8_trunc

        .area   _CODE

        ; inputs:  HL = fixed8_8
        ; outputs: DE = round(x)
_fixed8_8_round::
        bit     7,h
        jr      nz,.negative
        ld      de,#0x0080
        add     hl,de
        jp      _fixed8_8_trunc
.negative:
        ld      de,#0x0080
        or      a
        sbc     hl,de
        jp      _fixed8_8_trunc
