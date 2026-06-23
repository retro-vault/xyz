        ; fixed16_16_lround.s
        ;
        ; Round signed 16.16 to long/int result.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module fixed16_16_lround
        .optsdcc -mz80 sdcccall(1)

        .globl  _fixed16_16_lround
        .globl  _fixed16_16_round
        .globl  _fixed16_16_to_int

        .area   _CODE

_fixed16_16_lround::
        call    _fixed16_16_round
        call    _fixed16_16_to_int
        ld      hl,#0
        bit     7,d
        ret     z
        dec     hl
        ret
